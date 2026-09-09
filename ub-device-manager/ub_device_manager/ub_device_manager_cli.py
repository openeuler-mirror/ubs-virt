##########################################################################################################
# Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
#
# ub-device-manager is licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
##########################################################################################################
import inspect
import sys
import os
import json
import re

import httpx
import typer
from rich.console import Console
from rich.table import Table
from rich.tree import Tree
import yaml

# Strip system proxy environment variables at the process level
os.environ.pop("http_proxy", None)
os.environ.pop("https_proxy", None)
os.environ.pop("HTTP_PROXY", None)
os.environ["no_proxy"] = "localhost,127.0.0.1"
os.environ["NO_PROXY"] = "localhost,127.0.0.1"

UDS_PATH = os.getenv("UBDM_UDS_PATH", "/var/run/ub_device_manager/ub_device_manager.sock")

app = typer.Typer(help="\U0001F680 Ub Device Manager Control Center (CLI)")
console = Console()


def render_tree_data(data, parent_tree: Tree = None, root_label: str = "Response Data"):
    """Recursively render any dict/list as a tree mind map."""
    # Create the root node
    if parent_tree is None:
        root_tree = Tree(f"[bold green]{root_label}[/bold green]")
        render_tree_data(data, root_tree)
        console.print(root_tree)
        return

    # Recurse into branches
    if isinstance(data, dict):
        for k, v in data.items():
            branch = parent_tree.add(f"[cyan]{k}[/cyan]")
            render_tree_data(v, branch)
    elif isinstance(data, list):
        if len(data) == 0:
            parent_tree.add("[gray]空列表[/gray]")
            return
        # Number array items from 0, each with its own node to keep the hierarchy
        for index, item in enumerate(data):
            item_node = parent_tree.add(f"[yellow][{index}][/yellow]")
            render_tree_data(item, item_node)
    else:
        # Leaf node for plain string/number/bool/None values
        parent_tree.add(f"[white]{repr(data)}[/white]")


def get_server_client() -> httpx.Client:
    transport = httpx.HTTPTransport(uds=UDS_PATH, retries=0)
    return httpx.Client(
        base_url="http://ub-device-manager",
        timeout=300,
        proxy=None,
        transport=transport
    )


config_dir = os.path.dirname(os.path.abspath(__file__))
local_yaml_path = os.path.join(config_dir, "openapi.yaml")

if os.path.exists(local_yaml_path):
    with open(local_yaml_path, "r", encoding="utf-8") as f:
        try:
            openapi_spec = yaml.load(f, Loader=yaml.SafeLoader)
        except Exception as e:
            console.print(f"[bold red]\u274C Error: Failed to parse openapi.yaml. Reason: {e}[/bold red]")
            sys.exit(1)
else:
    console.print(f"[bold red]\u274C Error: Local contract file not found: {local_yaml_path}[/bold red]")
    sys.exit(1)


def resolve_schema(schema, components):
    if not schema or not isinstance(schema, dict):
        return schema
    if "$ref" in schema:
        ref_path = schema["$ref"].split("/")
        current = components
        for part in ref_path[2:]:
            current = current.get(part, {})
        return resolve_schema(current, components)
    return schema


def try_parse_json(val: str):
    if not isinstance(val, str):
        return val
    clean_val = val.strip()
    if (clean_val.startswith("'") and clean_val.endswith("'")) or (
            clean_val.startswith('"') and clean_val.endswith('"')):
        clean_val = clean_val[1:-1].strip()
    if clean_val.startswith('{') or clean_val.startswith('['):
        try:
            return json.loads(clean_val)
        except Exception:
            pass
    try:
        return json.loads(f"[{clean_val}]")[0]
    except Exception:
        pass
    return val


def get_param_python_type(type_str: str):
    t = type_str.lower()
    if t == "integer":
        return int
    elif t == "number":
        return float
    elif t == "boolean":
        return bool
    elif t == "array":
        return list
    elif t == "object":
        return dict
    elif t == "string":
        return str
    return str


def get_param_cli_type(type_str: str):
    t = type_str.lower()
    # 保留 bool 为原生 bool，仅 array/object 使用 str 以便自定义 JSON 解析
    if t in ("array", "object"):
        return str
    return get_param_python_type(type_str)


def parse_relaxed_json(raw: str):
    """Repair JSON whose quotes were stripped by the shell and parse it.

    Shells remove double quotes and split on spaces, so --cfg {"a": 1, "b": 2}
    arrives as fragments like '{a:' '1,' 'b:' '2}'. This joins them back into
    valid JSON by quoting bare keys and string values and re-inserting commas
    that brace expansion ate. Returns a dict/list, or None if the text cannot
    be repaired.
    """
    s = raw.strip()
    if not s:
        return None
    out = []
    i, n = 0, len(s)
    prev_struct = ""
    value_seen = False

    def emit_struct(ch):
        nonlocal prev_struct, value_seen
        out.append(ch)
        prev_struct = ch
        value_seen = ch in "}]"

    def emit_comma_if_needed():
        nonlocal prev_struct
        if value_seen and prev_struct != ",":
            out.append(",")
            prev_struct = ","

    while i < n:
        c = s[i]
        if c in "{}[],:":
            emit_struct(c)
            i += 1
        elif c.isspace():
            i += 1
        elif c in "'\"":
            j, buf = i + 1, []
            while j < n:
                ch = s[j]
                if ch == "\\" and j + 1 < n:
                    buf.append(ch)
                    buf.append(s[j + 1])
                    j += 2
                    continue
                if ch == c:
                    break
                buf.append(ch)
                j += 1
            if j >= n:
                return None
            out.append('"' + "".join(buf) + '"')
            i = j + 1
        else:
            j = i
            while j < n and s[j] not in "{}[],:" and not s[j].isspace() and s[j] not in "'\"":
                j += 1
            word = s[i:j]
            k = j
            while k < n and s[k].isspace():
                k += 1
            if k < n and s[k] == ":":
                emit_comma_if_needed()
                out.append(json.dumps(word))
                i = j
                continue
            emit_comma_if_needed()
            low = word.lower()
            if low in ("true", "false", "null"):
                out.append(low)
                value_seen = True
                i = j
                continue
            try:
                float(word)
                out.append(word)
                value_seen = True
                i = j
                continue
            except ValueError:
                pass
            parts = [word]
            j2 = j
            while True:
                k2 = j2
                while k2 < n and s[k2].isspace():
                    k2 += 1
                if k2 >= n or s[k2] in ",}]":
                    break
                if s[k2] in '{[:"\'':
                    break
                m2 = k2
                while m2 < n and s[m2] not in "{}[],:" and not s[m2].isspace() and s[m2] not in "'\"":
                    m2 += 1
                parts.append(s[k2:m2])
                j2 = m2
            out.append(json.dumps(" ".join(parts)))
            value_seen = True
            i = j2
    text = "".join(out)
    try:
        parsed = json.loads(text)
    except Exception:
        parsed = None
    if isinstance(parsed, (dict, list)):
        return parsed
    if text.startswith("{") and text.endswith("}"):
        try:
            parsed = json.loads("[" + text[1:-1] + "]")
        except Exception:
            return None
        return parsed if isinstance(parsed, list) else None
    return None


def parse_key_value_pairs(raw: str):
    """Parse 'key=value' pairs from comma/space separated text into a dict."""
    if "=" not in raw:
        return None
    result = {}
    current_key = None
    for token in re.split(r"[,\s]+", raw):
        if not token:
            continue
        if "=" in token:
            key, _, value = token.partition("=")
            result[key] = try_parse_json(value)
            current_key = key
        elif current_key is not None:
            result[current_key] = f"{result[current_key]} {token}"
        else:
            return None
    return result


def cast_and_validate_value(raw_val, target_type, param_name: str):
    if raw_val is None:
        return None
    val = try_parse_json(raw_val)

    # 增强对 bool 类型值的兼容解析（兼容原生 bool、字符以及数字）
    if target_type is bool:
        if isinstance(val, bool):
            return val
        if isinstance(val, str):
            lv = val.strip().lower()
            if lv in ("yes", "true", "True", "1", ""):
                return True
            elif lv in ("no", "false", "False", "0"):
                return False
            else:
                console.print(
                    f"[bold red]\u274C 参数 {param_name} 类型错误，需要布尔值(true/false/yes/no)，输入: {raw_val}[/bold red]")
                raise typer.Exit(1)
        if isinstance(val, int):
            return bool(val)

    if target_type is list:
        if isinstance(raw_val, list):
            return raw_val
        if isinstance(raw_val, str):
            stripped = raw_val.strip()
            if stripped.startswith(("[", "{")):
                parsed = try_parse_json(stripped)
                if isinstance(parsed, list):
                    return parsed
                repaired = parse_relaxed_json(stripped)
                if isinstance(repaired, list):
                    return repaired
                console.print(
                    f"[bold red]Parameter {param_name} type error: expected array, "
                    f"use comma separated values (--{param_name} 1,2,3) or quoted JSON, input: {raw_val}[/bold red]")
                raise typer.Exit(1)
            items = [item.strip() for item in re.split(r"[,\s]+", stripped) if item.strip()]
            if items:
                return [try_parse_json(item) for item in items]
        console.print(
            f"[bold red]Parameter {param_name} type error: expected array, "
            f"use comma separated values (--{param_name} 1,2,3) or quoted JSON, input: {raw_val}[/bold red]")
        raise typer.Exit(1)

    if target_type is dict:
        if isinstance(raw_val, dict):
            return raw_val
        if isinstance(raw_val, str):
            stripped = raw_val.strip()
            if stripped.startswith(("[", "{")):
                parsed = try_parse_json(stripped)
                if isinstance(parsed, dict):
                    return parsed
                repaired = parse_relaxed_json(stripped)
                if isinstance(repaired, dict):
                    return repaired
                console.print(
                    f"[bold red]Parameter {param_name} type error: expected object, "
                    f"use key=value pairs (--{param_name} a=1,b=2) or quoted JSON, input: {raw_val}[/bold red]")
                raise typer.Exit(1)
            pairs = parse_key_value_pairs(stripped)
            if pairs is not None:
                return pairs
        console.print(
            f"[bold red]Parameter {param_name} type error: expected object, "
            f"use key=value pairs (--{param_name} a=1,b=2) or quoted JSON, input: {raw_val}[/bold red]")
        raise typer.Exit(1)

    # Cast scalar values.
    try:
        return target_type(val)
    except (ValueError, TypeError):
        console.print(
            f"[bold red]\u274C 参数 {param_name} 类型错误，需要 {target_type.__name__}，输入值: {raw_val}[/bold red]")
        raise typer.Exit(1)


## Convert routes and HTTP methods to elegant CLI commands
def parse_route_to_cmd(path: str, method: str):
    path_parts = [p for p in path.split("/") if p]
    if not path_parts:
        return None, None

    # The first path level becomes the group name
    group_name = path_parts[0]

    # Single-level path, e.g. /ssu-vfe
    if len(path_parts) == 1:
        if method.lower() == "get":
            return group_name, "list"
        elif method.lower() == "post":
            return group_name, "create"
        else:
            return group_name, f"{method.lower()}"

    last_part = path_parts[-1]
    is_last_part_variable = last_part.startswith("{") and last_part.endswith("}")

    # Case 1: the path ends with a path variable
    if is_last_part_variable:
        if method.lower() == "get":
            return group_name, "show"
        elif method.lower() == "delete":
            return group_name, "delete"
        elif method.lower() in ["put", "patch"]:
            return group_name, "update"
        else:
            return group_name, f"{method.lower()}"
    # Case 2: the path ends with a plain action, use its last segment as the command name
    else:
        return group_name, last_part


components = openapi_spec.get("components", {})

sub_apps = {}
group_callbacks = {}
group_commands = []

for path, methods in openapi_spec.get("paths", {}).items():
    for method, details in methods.items():
        # Get the group name and command name via the conversion function
        sub_group_name, cmd_name = parse_route_to_cmd(path, method)

        if not sub_group_name or not cmd_name:
            continue

        summary = details.get("summary") or details.get("description") or f"Call {method.upper()} {path} API"
        extracted_params = {}

        api_parameters = details.get("parameters", [])
        for p in api_parameters:
            p_resolved = resolve_schema(p, components)
            p_name = p_resolved.get("name")
            p_in = p_resolved.get("in", "query")
            p_desc = p_resolved.get("description", f"{p_in} parameter: {p_name}")
            # Read the parameter schema and type
            p_schema = resolve_schema(p_resolved.get("schema", {}), components)
            p_type = p_schema.get("type", "string")
            extracted_params[p_name] = {
                "in": p_in,
                "description": p_desc,
                "required": p_resolved.get("required", False),
                "type": p_type
            }

        request_body = details.get("requestBody")
        if request_body:
            body_resolved = resolve_schema(request_body, components)
            schema = resolve_schema(body_resolved.get("content", {}).get("application/json", {}).get("schema"),
                                    components)
            if schema and schema.get("type") == "object":
                properties = schema.get("properties", {})
                required_list = schema.get("required", [])
                for prop_name, prop_detail in properties.items():
                    prop_resolved = resolve_schema(prop_detail, components)
                    prop_type = prop_resolved.get("type", "string")
                    extracted_params[prop_name] = {
                        "in": "body",
                        "description": prop_resolved.get("description", f"Body parameter: {prop_name}"),
                        "required": prop_name in required_list,
                        "type": prop_type
                    }

        context = {"path": path, "method": method, "params": extracted_params, "summary": summary}
        group_commands.append((sub_group_name, cmd_name, context))


def parse_error_response(resp_json: dict):
    """Parse the ErrorResponse structure and print messages + code"""
    code = resp_json.get("code", -1)
    msg = resp_json.get("msg", "未知错误")
    # Print concise error info directly for plain text
    console.print(f"\n[bold red]Request failed![/bold red]")
    console.print(f"messages: {msg}")
    console.print(f"code: {code}\n")


def build_executor(raw_path, req_method, api_params, help_text):
    def dynamic_execute(**kwargs):
        verbose = kwargs.pop("cli_verbose", False)
        ctx = kwargs.pop("ctx", None)

        if hasattr(kwargs, "_is_callback") and not any(
                v is not None for k, v in kwargs.items() if k not in ["cli_verbose", "_is_callback"]):
            return

        # Unquoted JSON like --cfg {"a": 1, "b": 2} is split by the shell into
        # several tokens; click collects the extras in ctx.args. Attach them to
        # the array/object parameter so the fragments can be re-joined.
        extra_args = list(getattr(ctx, "args", None) or []) if ctx else []
        if extra_args:
            greedy_names = [n for n, i in api_params.items() if i["type"] in ("array", "object")]
            if not greedy_names:
                console.print(f"[bold red]\\u274C Error: Unexpected extra argument: {' '.join(extra_args)}[/bold red]")
                raise typer.Exit(code=1)
            # Attach to the last array/object param that received a value.
            targets = [n for n in greedy_names if kwargs.get(n.replace("-", "_")) is not None]
            target = (targets or greedy_names)[-1]
            kw_name = target.replace("-", "_")
            existing = kwargs.get(kw_name)
            kwargs[kw_name] = f"{existing} {' '.join(extra_args)}" if existing is not None else " ".join(extra_args)

        final_path = raw_path
        query_params = {}
        body_payload = {}

        for param_name, info in api_params.items():
            safe_name = param_name.replace("-", "_")
            user_value = kwargs.get(safe_name)

            if info["required"] and user_value is None:
                console.print(f"[bold red]\u274C Error: Missing required parameter: {param_name}[/bold red]")
                raise typer.Exit(code=1)

            if user_value is not None:
                target_py_type = get_param_python_type(info["type"])
                processed_value = cast_and_validate_value(user_value, target_py_type, param_name)

                if info["in"] == "path":
                    final_path = final_path.replace(f"{{{param_name}}}", str(processed_value))
                elif info["in"] == "query":
                    query_params[param_name] = processed_value
                elif info["in"] == "body":
                    body_payload[param_name] = processed_value

        req_method_upper = req_method.upper()
        request_kwargs = {"method": req_method_upper, "url": final_path,
                          "params": query_params if query_params else None}

        if req_method_upper in ["POST", "PUT", "PATCH"]:
            if body_payload:
                request_kwargs["json"] = body_payload
        else:
            if body_payload:
                if not request_kwargs["params"]:
                    request_kwargs["params"] = {}
                request_kwargs["params"].update(body_payload)

        with console.status(f"[bold cyan]Sending request...", spinner="aesthetic"):
            try:
                if verbose:
                    console.print("\n[DEBUG] Request params:", request_kwargs)
                with get_server_client() as c:
                    res = c.request(**request_kwargs)
            except Exception as req_err:
                console.print(f"\n[bold red]\u274C Network request failed: {req_err}[/bold red]")
                raise typer.Exit(code=1)

        if str(res.status_code).startswith("20"):
            console.print("\n[bold green]\u2714 Execution successful. ")
            try:
                data_content = res.json()
                if isinstance(data_content, list) and len(data_content) > 0 and isinstance(data_content[0], dict):
                    table = Table(show_header=True, header_style="bold green")
                    headers = data_content[0].keys()
                    for h in headers:
                        table.add_column(str(h), style="cyan")
                    for item in data_content:
                        table.add_row(*[str(item.get(h, "")) for h in headers])
                    console.print(table)
                else:
                    render_tree_data(data_content)
            except Exception:
                console.print(res.text)
        else:
            try:
                parse_error_response({"code": res.status_code, "msg": res.json()})
                if verbose:
                    console.print(res.text)
            except Exception:
                console.print(res.text)
            raise typer.Exit(code=1)

    sig_parameters = []

    # Path params -> positional arguments (Argument)
    for param_name, info in api_params.items():
        if info["in"] == "path":
            safe_name = param_name.replace("-", "_")
            py_type = get_param_cli_type(info["type"])
            sig_parameters.append(
                inspect.Parameter(
                    safe_name, inspect.Parameter.KEYWORD_ONLY,
                    default=typer.Argument(..., help=info["description"]),
                    annotation=py_type
                )
            )

    # Non-path params -> options (Option)
    for param_name, info in api_params.items():
        if info["in"] != "path":
            safe_name = param_name.replace("-", "_")
            py_type = get_param_cli_type(info["type"])
            option_help = info["description"]

            if info["type"] in ("array", "object"):
                option_help += ' (JSON-like value, no quotes needed, e.g. {"a": 1, "b": 2})'
                default_val = ... if info["required"] else None
                param_option = typer.Option(default_val, f"--{param_name}", help=option_help)
            elif info["type"] == "boolean":
                param_option = typer.Option(
                    None,
                    f"--{param_name}/--no-{param_name}",
                    show_default=False,
                    help=f"{option_help} (support --{param_name} / --no-{param_name} or set true/false)"
                )
            else:
                default_val = ... if info["required"] else None
                param_option = typer.Option(default_val, f"--{param_name}", help=option_help)

            sig_parameters.append(
                inspect.Parameter(
                    safe_name, inspect.Parameter.KEYWORD_ONLY,
                    default=param_option,
                    annotation=py_type
                )
            )

    sig_parameters.append(
        inspect.Parameter("ctx", inspect.Parameter.KEYWORD_ONLY, annotation=typer.Context))

    sig_parameters.append(
        inspect.Parameter("cli_verbose", inspect.Parameter.KEYWORD_ONLY,
                          default=typer.Option(False, "--verbose", "-v"),
                          annotation=bool))

    dynamic_execute.__signature__ = inspect.Signature(sig_parameters)
    dynamic_execute.__doc__ = help_text
    return dynamic_execute


# Build and register all sub-apps
all_groups = set([g[0] for g in group_commands])

for group in all_groups:
    sub_apps[group] = typer.Typer(help=f"\U0001F4C2 {group} Subcommands")
    app.add_typer(sub_apps[group], name=group)

for group, cmd_name, cmd_ctx in group_commands:
    func = build_executor(cmd_ctx["path"], cmd_ctx["method"], cmd_ctx["params"], cmd_ctx["summary"])
    func.__name__ = cmd_name
    sub_apps[group].command(name=cmd_name, help=cmd_ctx["summary"],
                            context_settings={"allow_extra_args": True})(func)

if __name__ == "__main__":
    app()
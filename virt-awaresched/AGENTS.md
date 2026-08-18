# AGENTS.md

## Project Overview

Virt-awaresched (VSched) is an openEuler virtualization scheduling service for Kunpeng CPU topologies. It collects VM and host CPU topology information, assigns vCPU threads with cluster locality in mind, and supports static and dynamic affinity modes to reduce cross-cluster access and unnecessary vCPU migration.

This is a C++17/CMake project for openEuler Linux, primarily ARM64. The two shipped executables are:

- `vas_daemon`: the privileged scheduling daemon.
- `vasctl`: the administrator CLI, communicating with the daemon through `/var/run/vas/vas_uds.sock`.

## Build Commands

Run build commands from the repository root on openEuler/Linux.

```shell
# Release build (default)
bash build.sh

# Debug build
bash build.sh -D

# Build an RPM package
bash build.sh package

# Clean generated build state, then rebuild
bash build.sh -c
```

Required build dependencies include `gcc`, `gcc-c++`, `cmake`, `make`, `patch`, `libvirt-devel`, and `libboundscheck`. The build script uses the repository-local `build/` directory; executables are generated under `build/bin/`, and RPM output is copied to `output/`.

## Test Commands

```shell
# Build and run all unit tests
bash build.sh test

# Run a test suite
bash build.sh test -- --gtest_filter="TestClusterSched.*"

# Run one test case
bash build.sh test -- --gtest_filter="TestClusterSched.testUpdateDomainInfo1"

# Run unit tests and generate coverage data
bash build.sh test -C
```

Unit tests use GoogleTest and MockCpp. Add tests under the matching module in `test/` and register new test areas through the relevant `CMakeLists.txt`. Prefer the narrowest relevant filter while iterating, then run the full unit-test command before handoff.

## Code Style

- Use C++17 and the repository's `.clang-format` configuration.
- The formatting baseline is Google style with 4-space indentation, right-aligned pointers, and a 120-column limit.
- Match existing naming, error-return, logging, and ownership patterns in the surrounding module.
- Use the project logger from `src/log`; do not introduce ad hoc standard-output logging in daemon code.
- Define shared error codes in `src/include/error.h` and keep public APIs documented with Doxygen-style comments.
- Keep changes scoped. Do not mix scheduling behavior changes with unrelated cleanup.

## Development Environment Tips

- Linux/openEuler is the authoritative build and runtime environment. Windows inspection can validate text and diffs, but it cannot validate libvirt, cgroups, `/sys`, `/proc`, RPM packaging, or daemon behavior.
- CMake exports `compile_commands.json` in `build/`; point the C++ language server at that build directory.
- The daemon expects `qemu:///system`, the cpuset cgroup hierarchy, CPU/NUMA topology under `/sys`, and process information under `/proc`.
- Dynamic affinity additionally requires the target kernel's tidal-affinity support, `dynamic_affinity=enable` in `/proc/cmdline`, `cpuset.preferred_cpus`, and access to `/proc/sys/kernel/sched_util_low_pct`.
- Do not assume documentation alone proves kernel behavior; validate dynamic-affinity changes against the target openEuler kernel and a representative VM workload.
- Preserve the exact cgroup path semantics and distinguish static writes to `cpuset.cpus` from dynamic writes to both `cpuset.cpus` and `cpuset.preferred_cpus`.

## Architecture

```text
src/
|-- cli/                    # Command parsing, serialization, and UDS client/server support
|-- include/                # Shared definitions and error codes
|-- log/                    # Project logging
|-- util/                   # Common helpers
|-- vasctl/                 # Administrator CLI and command registration
`-- vasd/                   # Scheduling daemon
    |-- acquire/            # CPU topology, /proc data, libvirt VM data, VM events
    |-- api/                # Daemon command handlers
    |-- arg_parse/          # Daemon options and dynamic-affinity environment checks
    |-- cluster_sched/      # Allocation, compaction, reassignment, and cgroup binding
    |-- conf/               # Runtime configuration
    |-- looper/             # Main event/periodic scheduling loop
    `-- security/           # Privilege and security checks

test/
|-- cli/                    # CLI-related unit tests
|-- log/                    # Logging unit tests
|-- util/                   # Utility unit tests
`-- vasd/                   # Daemon module unit tests
```

The main runtime flow is:

```text
vas_daemon startup
  -> initialize logging, privileges, arguments, libvirt, and CPU topology
  -> collect VM/vCPU state and listen for libvirt lifecycle events
  -> ClusterSched selects NUMA/cluster-local CPU assignments
  -> write assignments to VM vCPU cgroup files
  -> handle vasctl commands over the Unix domain socket
  -> periodically compact or reschedule allocations
```

Use `docs/design/ARCHITECTURE.md`, `docs/config/CONFIG.md`, `docs/cli/`, and `docs/test/TEST.md` for feature-specific details. When documentation and implementation differ, treat the current source and CMake wiring as authoritative and call out the mismatch.

## Security Guidelines

**Prohibited:**

1. Do not commit passwords, tokens, private keys, certificates, personal data, or credential-bearing connection strings.
2. Do not hard-code credentials or environment-specific secrets.
3. Do not weaken TLS, authentication, authorization, privilege, path, or input validation checks.
4. Do not log credentials, sensitive VM data, or other secrets.
5. Do not broaden daemon or installed-file permissions without an explicit security review.

**Required:**

- Validate external input and preserve bounds-checked operations.
- Keep the UDS and installed-file permission model least-privileged.
- Treat changes touching root execution, libvirt, cgroups, `/proc`, `/sys`, command parsing, or package installation as security-sensitive.
- Never replace runtime paths with developer-machine-specific absolute paths.

## Validation and Handoff

- For source changes, run `bash build.sh` and `bash build.sh test` on openEuler/Linux.
- For focused test changes, record the exact GoogleTest filter used and still run the full suite before handoff when the environment permits.
- For packaging changes, run `bash build.sh package`, inspect the generated RPM, and verify its digest and file list.
- For scheduling or affinity changes, supplement unit tests with runtime checks against libvirt, the target cgroup layout, and representative `/sys` and `/proc` state.
- If validation is limited to Windows or static inspection, state that boundary explicitly; do not describe the change as runtime-verified.

## Commit Guidelines

- Follow Conventional Commits: `<type>(<scope>): <subject>`.
- Typical types are `feat`, `fix`, `docs`, `test`, `refactor`, `ci`, and `perf`.
- Keep each commit focused and include tests or documentation when behavior or interfaces change.
- Before committing, format changed C/C++ files with `clang-format`, run the applicable build/tests, and confirm no sensitive or generated files are staged.

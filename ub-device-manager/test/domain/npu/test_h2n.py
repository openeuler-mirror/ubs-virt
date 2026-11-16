"""H2N sysfs probe and wait tests."""

from pathlib import Path
from unittest.mock import patch

import pytest

from ub_device_manager.domain.npu.h2n import _probe, wait_h2n_healthy


def _make_pfe(root: Path, name: str, guid: str, state: str) -> None:
    device = root / name
    device.mkdir()
    (device / "guid").write_text(guid, encoding="utf-8")
    (device / "fault_mgr_state").write_text(state, encoding="utf-8")


def test_probe_requires_every_pfe_healthy(tmp_path):
    _make_pfe(tmp_path, "ub0", "guid-a", "PF/IDLE")
    _make_pfe(tmp_path, "ub1", "guid-b", "FAULT")

    assert _probe(["guid-a"], tmp_path)[0] is True
    assert _probe(["guid-a", "guid-b"], tmp_path)[0] is False


async def test_wait_returns_after_stable_healthy_period(tmp_path):
    _make_pfe(tmp_path, "ub0", "guid-a", "PPF/IDLE")
    await wait_h2n_healthy(["guid-a"], {
        "sysfs_root": str(tmp_path), "wait_timeout": 1,
        "probe_timeout": 0.2, "poll_interval": 0.01,
        "healthy_stable_duration": 0.02,
    })


async def test_wait_times_out_when_link_unhealthy(tmp_path):
    _make_pfe(tmp_path, "ub0", "guid-a", "FAULT")
    with pytest.raises(TimeoutError, match="H2N wait timed out"):
        await wait_h2n_healthy(["guid-a"], {
            "sysfs_root": str(tmp_path), "wait_timeout": 0.04,
            "probe_timeout": 0.02, "poll_interval": 0.01,
            "healthy_stable_duration": 0,
        })


@pytest.mark.parametrize("first_result, first_detail", [
    ((False, "PFE guid-a state is FAULT"), "PFE guid-a state is FAULT"),
    (RuntimeError("mock probe failure"), "H2N probe failed: mock probe failure"),
])
async def test_wait_logs_each_probe_result_and_recovers_after_one_failure(
        tmp_path, first_result, first_detail):
    config = {
        "sysfs_root": str(tmp_path), "wait_timeout": 1,
        "probe_timeout": 0.2, "poll_interval": 0.01,
        "healthy_stable_duration": 0,
    }
    with patch("ub_device_manager.domain.npu.h2n._probe", side_effect=[
        first_result, (True, "All PFE links are healthy"),
    ]) as mock_probe, patch("ub_device_manager.domain.npu.h2n.logger.debug") as mock_debug:
        await wait_h2n_healthy(["guid-a"], config)

    assert mock_probe.call_count == 2
    assert mock_debug.call_count == 2
    assert mock_debug.call_args_list[0].args[1:] == (1, False, ["guid-a"], first_detail)
    assert mock_debug.call_args_list[1].args[1:] == (2, True, ["guid-a"], "All PFE links are healthy")

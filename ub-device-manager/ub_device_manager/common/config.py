import os
import tomllib

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONFIG_PATH = os.path.join(BASE_DIR, "ub_device_manager.toml")

def load_settings() -> dict:
    with open(CONFIG_PATH, "rb") as f:
        return tomllib.load(f)

CONFIG = load_settings()
Import("env")

board_config = env.BoardConfig()
# should be array of VID:PID pairs
board_config.update("build.hwids", [
  ["0x0483", "0x1234"]
])
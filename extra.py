# Overwritten flags to build a firmware for MCU devices
Import("env")
env.Append(
  LINKFLAGS=[
    "-mfpu=fpv4-sp-d16",
    "-mfloat-abi=hard",
    "-mthumb"
  ]
)
env.Append(
  CCFLAGS=[
    "-mfpu=fpv4-sp-d16",
    "-mfloat-abi=hard",
    "-mthumb"
  ]
)
print(env.Dump())
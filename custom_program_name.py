Import("env")

# Get the environment name (e.g., "radio", "launcher")
env_name = env["PIOENV"]

# Replace the default program name with the environment name.
# This will affect the .elf, .bin, and other output files.
env.Replace(PROGNAME=env_name)
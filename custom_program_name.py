import os
Import("env")

# Get the current build environment name
env_name = env.subst("$PIOENV")

# Replace the default program name with the environment name
env.Replace(PROGNAME=env_name)

# Optional but helpful: print a confirmation message during the build
print(f"Building with program name: {env_name}")

import os
Import("env")

# Get the custom program name from the platformio.ini file for the current environment
# The 'default' is a fallback, but shouldn't be needed with your setup
prog_name = env.GetProjectOption("custom_prog_name", "firmware")

# Replace the default program name with our custom one
env.Replace(PROGNAME=prog_name)

# Optional but helpful: print a confirmation message during the build
print(f"Building with custom program name: {prog_name}")

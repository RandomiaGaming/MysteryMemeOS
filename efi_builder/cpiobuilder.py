#!/bin/python3



# Libraries
import sys
import glob
import os
import subprocess



# Helpers
def error(message):
    print(f"\x1b[91mERROR: {message}\x1b[0m\n")
    exit(1)
def cmd(command):
    subprocess.run(command, shell=True, check=True)



# Config
outdir = None
outfile = None
compress = None
confmode = True
def conf_command(args):
    global outdir
    global outfile
    global compress
    i = args.find(":")
    if i == -1:
        error(f"No value provided when attempting to set {key}.")
    key = args[:i].strip()
    value = args[i + 1:].strip()
    if ":" in value:
        error(f"Too many values provided when attempting to set {key}.")
    if key == "outdir":
        outdir = value
    elif key == "outfile":
        outfile = value
    elif key == "compress":
        if value == "yes":
            compress = True
        elif value == "no":
            compress = False
        else:
            error("Invalid value provided for compress. Options are yes or no.")
    else:
        error(f"Unknown config setting {key}.")
def exit_conf_mode():
    global confmode
    confmode = False
    global outdir
    global outfile
    global compress
    if outdir == None:
        outdir = "initramfs"
    if compress == None:
        compress = True
    if outfile == None:
        if compress:
            outfile = "initramfs.img.gz"
        else:
            outfile = "initramfs.img"
    print(f"Settings outdir={outdir} compress={compress} outfile={outfile}")
    cmd(f"rm -rf {outdir}")
    cmd(f"mkdir {outdir}")



# Commands
def dir_command(args):
    i = args.find(":")
    if i == -1:
        destination = args.strip()
        cmd(f"mkdir {outdir}{destination}")
    else:
        source = args[:i].strip()
        destination = args[i + 1:]
    if source == "":
        error(f"No value provided for source.")
    if ":" in destination:
        error(f"Too many arguments provided.")
    if destination == "":
    else:
        cmd(f"cp -r {source} {outdir}{source}")
def link_command(args):
    i = args.find(":")
    if i == -1:
        source = args.strip()
        destination = ""
    else:
        source = args[:i].strip()
        destination = args[i + 1:]
    cmd(f"ln -s {destination} {outdir}{source}")
def bin_command(args):
    print(args)
def lib_command(args):
    print(args)
def mod_command(args):
    print(args)
def file_command(args):
    print(args)



# Main
def main():
    # Read recipe and switch to its directory
    recipePath = ""
    if len(sys.argv) == 2:
        recipePath = sys.argv[1]
    elif len(sys.argv) <= 1:
        recipePaths = glob.glob("./*.recipe")
        if len(recipePaths) == 1:
            recipePath = recipePaths[0]
        elif len(recipePaths) > 1:
            error(f"Ambiguaty between recipe files {recipePaths}.")
        else:
            error(F"No recipe file specified. Try cpiobuilder /path/to/example.recipe")
    else:
        error("Too many arguments. Usage cpiobuilder /path/to/example.recipe")
    recipePath = os.path.abspath(recipePath)
    if not os.path.exists(recipePath):
        error(f"Recipe file {recipePath} does not exist.")
    recipeFile = open(recipePath, "r")
    recipe = recipeFile.read()
    recipeFile.close()
    os.chdir(os.path.dirname(recipePath))
    # Process commands one by one
    for line in recipe.split("\n"):
        if line.strip() == "":
            continue
        if line.startswith("#"):
            continue
        i = line.find(":")
        if i == -1:
            command = line.strip()
            args = ""
        else:
            command = line[:i].strip()
            args = line[i + 1:]
        if command == "conf":
            if not confmode:
                error("conf commands must preceed all other commands.")
            conf_command(args)
        else:
            if confmode:
                exit_conf_mode()
            if command == "dir":
                dir_command(args)
            elif command == "link":
                link_command(args)
            elif command == "bin":
                bin_command(args)
            elif command == "lib":
                lib_command(args)
            elif command == "mod":
                mod_command(args)
            elif command == "file":
                file_command(args)
            else:
                error(f"Unknown command {command}.")
    exit(0)
main()
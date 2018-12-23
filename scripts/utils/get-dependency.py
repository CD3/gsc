#! /usr/bin/env python

config_filename = ".get-dependency.rc"

import subprocess,json,os,sys,tempfile
from argparse import ArgumentParser

parser = ArgumentParser(description="A program to retrieve dependencies.")

parser.add_argument("dependency",
                    action="store",
                    nargs='*',
                    default=[],
                    help="Dependencies to retrieve" )

parser.add_argument("-c", "--config",
                    action="store",
                    help="Config file to use.")

args = parser.parse_args()


qx = lambda cmd : subprocess.check_output( cmd, shell=True, stderr=subprocess.STDOUT )

project_dir = qx("git rev-parse --show-toplevel").strip()

paths = []
paths.append(os.environ["HOME"])
paths.append(project_dir)
paths.append("./")
config = dict()
for dir in paths:
  cf = os.path.join( dir, config_filename )
  if os.path.exists(cf):
    with open(cf) as f:
      config.update( json.load(f) )


devnull = open(os.devnull,'w')
tmpdir=tempfile.mkdtemp()
for dep in args.dependency:
  found=False
  if 'git' in config:
    if 'remotes' in config['git']:
      url=None
      for remote in config['git']['remotes']:
        candidate=remote+dep
        cmd="git ls-remote --exit-code %s"%candidate
        ret = subprocess.call(cmd,stdout=devnull,stderr=subprocess.STDOUT,shell=True)
        if ret == 0:
          print dep,"found at",candidate
          found = 1
          url=candidate
          break

      if found:
        dest = os.path.join(tmpdir,dep)
        cmd = "git clone %s %s"%(url,dest)
        ret = subprocess.call(cmd,shell=True)


  if not found:
    print("ERROR: Could not find %s"%dep)

devnull.close()

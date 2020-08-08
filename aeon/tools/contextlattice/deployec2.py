# chuangw: This script uses EC2 API to find the list of nodes.
# It then pssh to these nodes and download the executables and scripts, 
# finally, execute it.

import os
from libcloud.compute.types import Provider
from libcloud.compute.providers import get_driver

import argparse

# set up variables
EC2_ACCESS_ID = 'AKIAJBLYETW5LV7JPO2Q'
EC2_SECRET_KEY = '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'

http_server = "https://galba.cs.purdue.edu:54321/"

Driver = get_driver(Provider.EC2)
conn = Driver(EC2_ACCESS_ID, EC2_SECRET_KEY)

nodes = conn.list_nodes()


for n in nodes:
  print "Instance name: " + n.name + " IP: " + n.public_ips[0];
  
class Deploy( argparse.Action ):
  def __call__(self, parser, namespace, values, option_string=None):
  
    nodeips = " ".join( str(ip.public_ips[0]) for ip in nodes )

    # download execuable
    command = "curl " + http_server + "firstTag"

    dlApplication = "pssh -H \"" + nodeips + "\" -l ubuntu -O \"IdentityFile ~/.ssh/contextlattice.pem\" " + command
    os.system( dlApplication );

    # download parameter file
    command = "curl " + http_server + "params.tag"

    dlApplication = "pssh -H \"" + nodeips + "\" -l ubuntu -O \"IdentityFile ~/.ssh/contextlattice.pem\" " + command
    os.system( dlApplication );

    # launch process
    command = "./firstTag params.tag"
    dlApplication = "pssh -H \"" + nodeips + "\" -l ubuntu -O \"IdentityFile ~/.ssh/contextlattice.pem\" " + command
    os.system( dlApplication );
# command line option parser
parser = argparse.ArgumentParser(description='Tool for deploying and launching nodes in EC2.')
parser.add_argument('--listnode', nargs='?', default='a',dest='option_list', help="List EC2 nodes. Don't do anything else.")
parser.add_argument('--deploy', nargs='?', default='a', action=Deploy, help="Deploy executables and parameter files to the EC2 nodes.")
args = parser.parse_args()


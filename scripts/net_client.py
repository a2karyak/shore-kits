#!/usr/bin/python
"""
Shore-Kits network client that is capable of handling multiple instances simultaneously.

Miguel Branco - miguel.branco@epfl.ch
"""

"""
To Do:
- Kill process after last result, if it seems hanged.
- Move configurations/runs settings to separate config file
"""

import commands
import getopt
import glob
import os
import random
import re
import signal
import shutil
import socket
import sys
import time

# Settings are specified in KBs
MB = 1024 
GB = 1024*MB

"""
Tests suggested by Ippokratis:
- Use TPCC, TPCB, TM, with SF 1, 2, 4, 16, then 10, 20, 40, 160.
- Vary number of clients accordingly, so that they match number of cores
- Measure for at least 30 seconds w/ 3 iterations
- Use MIX for all workloads, plus TM1-GetSubData, TPCC-NewOrder (1) and TPCC-Payment (2)
- Use SLI, ELR, baseline and normal design
(Note: Should check if tpcc_input.c and all are working with local transactions only...)
"""

CONFIGURATIONS = {
	'tpcc-1': {	'sf': 1, 'devicequota': 2*GB, 'bufpoolsize': 2*GB, 'logsize': 2*GB, 'logbufsize': 80*MB,
			'system': 'baseline', 'benchmark': 'tpcc', 'design': 'normal' },
	'tpcc-2': {	'sf': 2, 'devicequota': 2*GB, 'bufpoolsize': 2*GB, 'logsize': 2*GB, 'logbufsize': 80*MB,
			'system': 'baseline', 'benchmark': 'tpcc', 'design': 'normal' },
	'tpcc-4': {	'sf': 4, 'devicequota': 2*GB, 'bufpoolsize': 2*GB, 'logsize': 2*GB, 'logbufsize': 80*MB,
			'system': 'baseline', 'benchmark': 'tpcc', 'design': 'normal' },
	'tpcc-16': {	'sf': 16, 'devicequota': 12*GB, 'bufpoolsize': 4*GB, 'logsize': 4*GB, 'logbufsize': 80*MB,
			'system': 'baseline', 'benchmark': 'tpcc', 'design': 'normal' },
}

RUNS = {
	# <run name>: (<configuration>, <nr processes>, [<list of commands>])
	'tpcc-1-1':	('tpcc-1',	[[i] for i in xrange(0, 16)],			['sli', 'elr', 'measure 1 1 1 30 1 3']),

	'tpcc-2-2':	('tpcc-2',	[[i,i+1] for i in xrange(0, 16, 2)],		['sli', 'elr', 'measure 2 1 2 30 1 3']),

	'tpcc-4-4':	('tpcc-4',	[[i,i+1,i+2,i+3] for i in xrange(0, 16, 4)],	['sli', 'elr', 'measure 4 1 4 30 1 3']),

	'tpcc-16-16':	('tpcc-16',	[[i for i in xrange(0, 16)]], 			['sli', 'elr', 'measure 16 1 16 30 1 3']),
}


# Default value for template file
SHORE_CONF_TEMPLATE = 'shore.conf.template'

# Default value for output log file
OUTPUT_LOG = 'output.log'

# Default value for shore_kits executable
SHORE_PATH = os.getcwd()+'/..'

# Default value for temporary directory
TEMPDIR = '/tmpfs/%s' % os.getenv('USER')

# Default value for port range
PORT_RANGE = (5000, 6000)


class ShoreException(Exception):
	pass
	

class ShoreInstance(object):

	def __init__(self, template, path, port, cfgname, affinity, temp):
		self.template = template
		self.path = path
		self.port = port
		self.cfgname = cfgname
		self.affinity = affinity

		if not os.path.exists(temp):
			raise ShoreException('Temporary directory %s does not exist' % temp)
		if not os.path.exists('%s/shore_kits' % self.path):
			raise ShoreException('Cannot find shore_kits in %s' % self.path)	
		if not os.path.exists(self.template):
			raise ShoreException('Cannot find template file %s' % self.template)

		self.dir = '%s/%s-%s' % (temp, self.cfgname, self.port)

		if not os.path.exists(self.dir):
			os.makedirs(self.dir)
		elif glob.glob('%s/*' % self.dir):
			raise ShoreException('Temporary directory %s is not empty' % self.dir)

		self.output = '%s/shore.log' % self.dir
		self.cfgfile = '%s/shore.conf' % self.dir

		self.devicedir = '%s/databases' % self.dir
		os.makedirs(self.devicedir)

		self.logdir = '%s/log-%s' % (self.dir, self.cfgname)
		os.makedirs(self.logdir)

		self.__generate_config()


	def __generate_config(self):
	        try:
        	        shutil.copyfile(self.template, self.cfgfile) 
	        except:
	                raise ShoreException('Failed copying template %s to %s' % (self.template, self.cfgfile))

        	try:
	                f = open(self.cfgfile, 'a')
	                f.write('\n\n# AUTO-GENERATED ENTRIES\n')
			f.write('db-workers = %d\n' % len(self.affinity))
			f.write('db-loaders = %d\n' % len(self.affinity))
	                f.write('db-config = %s\n' % self.cfgname)
	                f.write('%s-device = %s/db-%s\n' % (self.cfgname, self.devicedir, self.cfgname))
	                f.write('%s-logdir = %s\n' % (self.cfgname, self.logdir))
	                for k in CONFIGURATIONS[self.cfgname]:
	                        v = CONFIGURATIONS[self.cfgname][k]
	                        f.write('%s-%s = %s\n' % (self.cfgname, k, v))
	                f.close()
	        except:
	                raise ShoreException('Failed generating %s' % self.cfgfile)


	def spawn(self):
		self.pid = os.fork()
		if self.pid == 0:
			# child

			affinity = ','.join([str(k) for k in self.affinity])
			s, o = commands.getstatusoutput('cd %s; taskset -c %s %s/shore_kits -n -p %d > %s 2>&1' % (self.dir, affinity, self.path, self.port, self.output))

			f = open(self.output, 'a')
			if s != 0:
				f.write('\nFAILED\n')
			f.close()
			sys.exit(0)


	def wait_start(self):
		"""Wait until server is ready to receive connections."""
		while True:
			time.sleep(1)
			f = open(self.output, 'r')
			lines = f.readlines()
			f.close()

			if not lines: continue
		
			if 'Waiting for client connection' in lines[-1]:
				break
			if 'FAILED' in lines[-1]:
				raise ShoreException('Failed spawning shore instance. Check %s for details.' % self.output)
		

	def connect(self):
		"""Connect to server."""
		self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.socket.connect(('localhost', self.port))


	def send(self, cmd):
		"""Send command to server."""
		self.socket.send('%s\n' % cmd)


	def stop(self):
		"""Ask server to stop."""
		self.send('quit\n')
		self.send('')
		self.socket.close()


	def wait_stop(self):
		"""Wait until server is done."""
		os.waitpid(self.pid, 0)


	def getoutput(self):
		f = open(self.output, 'r')
		try: return f.readlines()
		finally: f.close()


	def clean(self):
		try: shutil.rmtree(self.dir)
		except: pass


	def kill(self):
		"""Kill child."""
		os.kill(self.pid, signal.SIGKILL)
		return self.pid



def parse_shore_log(log):
	results = {}

	for l in log:
		for key, regex in [	('TPS', 'MQTh/s\:[ ]*\((?P<data>[0-9.]*)\)'),
					('TPS', 'TPS:[ ]*\((?P<data>[0-9.]*)\)'), ]:
			r = re.search(regex, l)
			if r:
				results.setdefault(key, [])
				results[key].append( r.group('data') )

	return results


ShoreInstances = []
ParentPID = os.getpid()


def sigint_handler(signum, frame):
	global ShoreInstances

	if os.getpid() != ParentPID:
		sys.exit()

	print 'CTRL-C caught'
	for s in ShoreInstances:
		print 'Sent SIGKILL to %s' % s.kill()
	sys.exit()


def main(output, template, path, port_range, temp):
	global ShoreInstances

	signal.signal(signal.SIGINT, sigint_handler)

	runs = RUNS.keys()
	runs.sort()
	for run in runs:
		cfgname, affinity, cmds = RUNS[run]

		print 'Run %s' % run

		ShoreInstances = []
		for i in xrange(0, len(affinity)):
			port = random.randint(port_range[0], port_range[1])
			ShoreInstances.append( ShoreInstance(template, path, port, cfgname, affinity[i], temp) )

		for i, s in enumerate(ShoreInstances):
			print 'Starting server at cores %s...' % affinity[i]
			s.spawn()

		for i, s in enumerate(ShoreInstances):
			print 'Waiting for server at cores %s to initialize...' % affinity[i]
			s.wait_start()

		for i, s in enumerate(ShoreInstances):
			print 'Connecting to server at cores %s...' % affinity[i]
			s.connect()

		print 'Sending commands to servers...'
		for s in ShoreInstances:
			for c in cmds:
				s.send(c)
			s.stop()

		f = open(output, 'a')
		f.write('Run\t%s\n' % run)
		f.close()

		for i, s in enumerate(ShoreInstances):
			print 'Waiting for server at cores %s to finish...' % affinity[i]
			s.wait_stop()
	
			results = parse_shore_log(s.getoutput())

			f = open(output, 'a')
			for k, v in results.items():
				f.write('%s\t%s\n' % (k, '\t'.join(v)))
			f.close()

			s.clean()
	

def usage():
	print """Usage:
 -o filename  : Output log file to use for results (Default: %s)
 -t template  : Template file to use for generating shore.conf (Default: %s)
 -p path      : Location of shore_kits executable (Default: %s)
 -T dir       : Temporary directory to use for database (Default: %s)
 -P min,max   : Specify port range to use, comma-separated (Default: %s)"""  % \
	(OUTPUT_LOG, SHORE_CONF_TEMPLATE, SHORE_PATH, TEMPDIR, ','.join([str(p) for p in PORT_RANGE]))


if __name__ == '__main__':
	try:
		opts, args = getopt.getopt(sys.argv[1:], "ho:t:p:T:P:", [])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		sys.exit(2)
	output = OUTPUT_LOG
	template = SHORE_CONF_TEMPLATE
	path = SHORE_PATH
	temp = TEMPDIR
	port_range = PORT_RANGE
	for o, a in opts:
		if o == '-h':
			usage()
			sys.exit()
		elif o == '-o':
			output = a
		elif o == '-t':
			template = a
		elif o == '-p':
			path = a
		elif o == '-T':
			temp = a
		elif o == '-P':
			port_range = [int(p) for p in a.split(',')]
		else:
			assert False, "unhandled option"
	print 'Running with settings:'
	print '  Output log file: %s' % output
	print '  Template file: %s' % template
	print '  Location of shore_kits executable: %s' % path
	print '  Temporary directory: %s' % temp
	print
	try:
		main(output, template, path, port_range, temp)
	except ShoreException, e:
		print
		print 'ERROR: %s' % e
		print '       (Run script with -h to view configuration options.)'




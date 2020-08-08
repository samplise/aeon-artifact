import os
import sys
import subprocess


def runCommand(cmd, debug=False, blocking=True, stdoutFile=None, stderrFile=None, getStdout=False, getStderr=False):
    if debug:
        print cmd
        return (None, None, -1)

    if getStdout and stdoutFile is not None:
        print "Can't get stdout and have it redirect to a file at the same time"
        assert(false)

    if getStderr and stderrFile is not None:
        print "Can't get stderr and have it redirect to a file at the same time"
        assert(false)

    stdOutLoc = None

    outFP = None
    errFP = None

    if stdoutFile is not None and stderrFile is not None \
            and stdoutFile == stderrFile:
        outFP = open(stdoutFile, 'w')
        errFP = outFP

    if stdoutFile is not None:
        if outFP is None:
            outFP = open(stdoutFile, 'w')
        stdOutLoc = outFP
    elif getStdout == True:
        stdOutLoc = subprocess.PIPE

    stdErrLoc = None

    if stderrFile is not None:
        if errFP is None:
            errFP = open(stderrFile, 'w')
        stdErrLoc = errFP
    elif getStderr == True:
        stdErrLoc = subprocess.PIPE

    cmdObj = subprocess.Popen(cmd, shell=True, stdout=stdOutLoc, 
                              stderr=stdErrLoc)

    if blocking:
        (stdoutData, stderrData) = cmdObj.communicate()

        if stdoutData is not None:
            stdoutData = stdoutData.split('\n')

        if stderrData is not None:
            stderrData = stderrData.split('\n')

        returncode = cmdObj.returncode

        if returncode is not 0:
            print "Subprocess returned unsuccessful code %d" % returncode
            print "stderr: %s" % stderrData

        if outFP is not None:
            outFP.close()

        if errFP is not None:
            errFP.close()

        return (stdoutData, stderrData, returncode)
    else:
        return cmdObj

def runPipeCommand(cmd, blocking=False, debug=False):
    assert(cmd.find('|') != -1)
    
    if debug:
        print cmd
        return (None, None)

    pipedCommands = cmd.split('|')

    pipes = [None for x in xrange(len(pipedCommands))]
    pipes[0] = subprocess.Popen(pipedCommands[0], shell=True, stdout=subprocess.PIPE)

    for i in xrange(1, len(pipedCommands)):
        pipes[i] = subprocess.Popen(pipedCommands[i], stdin=pipes[i-1].stdout, 
                                    shell=True, stdout=subprocess.PIPE)

    if blocking:
        pipes[len(pipedCommands) - 1].wait()

    return (pipes[len(pipedCommands) - 1].stdout, pipes[len(pipedCommands) -1].stderr)

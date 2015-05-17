''' Measure memory usage  of PID each second. '''

import sys
import time

def get_status_file(pid):
    ''' Open the file that has the memory usage info. '''
    try:
        return open('/proc/{}/status'.format(pid))
    except FileNotFoundError:
        return None


def main():
    ''' Our main function. '''
    while True:
        with get_status_file(sys.argv[1]) as status_file:
            if status_file == None:
                sys.exit(0)
            #VmSize:  3201152 kB
            #VmRSS:   1530084 kB
            measures = {}
            for line in status_file:
                if line.startswith('VmSize:') or line.startswith('VmRSS:'):
                    measures[line.split()[0][:-1]] = line.split()[1]
        print(time.time(), measures['VmRSS'], measures['VmSize'])
        time.sleep(1)
    return 0

if __name__ == '__main__':
    sys.exit(main())

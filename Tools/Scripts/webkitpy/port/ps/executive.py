import logging

from webkitpy.common.system.executive import Executive

try:
    from executive_imp import PlayStationExecutiveImp
except:

    class PlayStationExecutiveImp():
        def __init__(self, host, device, port_name):
            self.host = host
            self.device = device
            self.port_name = port_name

        def cpu_count(self):
            return 1

        def kill_process(self):
            return

        def popen(self, args, **kwargs):
            return None

_log = logging.getLogger(__name__)


class PlayStationExecutive(Executive):
    def __init__(self, host, device, port_name, *args):
        super(PlayStationExecutive, self).__init__(*args)
        self.host = host
        self.device = device
        self.target = None
        self.last_elf_pid = None
        self._implement = PlayStationExecutiveImp(host, device, port_name)

    def cpu_count(self):
        return self._implement.cpu_count()

    def kill_process(self, pid):
        self._implement.kill_process()
        super(PlayStationExecutive, self).kill_process(pid)

    def popen(self, args, **kwargs):
        pop = self._implement.popen(args, **kwargs)
        if pop != None:
            self.last_elf_pid = pop.pid
            return pop
        return super(PlayStationExecutive, self).popen(args, **kwargs)

import logging
import os
import subprocess
import threading
import re

from webkitpy.port.ps.filesystem import PlayStationFileSystem
from webkitpy.port.ps.executive import PlayStationExecutive

try:
    from device_imp import PlayStationDeviceImp
except:

    class PlayStationDeviceImp():
        def __init__(self, worker, executive, port_number):
            self.worker = worker

        def get_target(self):
            return ''

_log = logging.getLogger(__name__)


class PlayStationDevice:
    def __init__(self, port, host, worker, port_name):
        self.listening_socket = None
        self.port = port
        self.host = host
        self._executive = PlayStationExecutive(host, self, port_name)
        self._filesystem = PlayStationFileSystem()
        self.worker = worker
        self._implement = PlayStationDeviceImp(worker, self.executive, port_name)

    def prepare_for_testing(self, ports_to_forward, layout_test_directory):
        self._implement.prepare_for_testing(ports_to_forward, layout_test_directory)
        return

    def finished_testing(self):
        return

    def symbolicate_crash_log_if_needed(self, path):
        return None

    def release_worker_resources(self):
        return None

    @property
    def executive(self):
        return self._executive

    @property
    def filesystem(self):
        return self._filesystem

    @property
    def user(self):
        return self.host.user

    @property
    def platform(self):
        return self.host.platform

    @property
    def workspace(self):
        return self.host.workspace

    @property
    def device_type(self):
        return None

    def __nonzero__(self):
        return True

    def __eq__(self, other):
        return self.worker == other.worker

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.worker)

    def getTarget(self):
        return self._implement.get_target()

    def working_directory(self):
        return self.port.working_directory()

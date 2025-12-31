import os
import uuid
import logging
import sys
import traceback

from distutils import spawn
from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.port.base import Port
from webkitpy.port.ps.device import PlayStationDevice
from webkitpy.port.ps.driver import PlayStationDriverProxy


_log = logging.getLogger(__name__)


class PlayStationPort(Port):
    port_name = "playstation"

    def __init__(self, *args, **kwargs):
        super(PlayStationPort, self).__init__(*args, **kwargs)
        self._device_for_worker_number_map = dict()
        self._port_name = kwargs.get('options').platform

    def _build_path(self, *comps):
        """Returns the full path to the test driver (DumpRenderTree)."""
        root_directory = self.get_option('_cached_root') or self.get_option('root')
        if not root_directory:
            Port._build_path(self, *comps)  # Sets option _cached_root
            binary_directory = 'bin'
            root_directory = self._filesystem.join(self.get_option('_cached_root'), binary_directory, self.get_option('configuration'))
            self.set_option('_cached_root', root_directory)

        return self._filesystem.join(root_directory, *comps)

    def check_image_diff(self, override_step=None, logging=True):
        image_diff_path = self._path_to_image_diff()
        if not self._filesystem.exists(image_diff_path):
            if logging:
                _log.error("ImageDiff was not found at %s, but ignore." % image_diff_path)
            return True
        return True

    def _path_to_apache(self):
        root = os.environ.get('XAMPP_ROOT', 'C:\\xampp')
        path = self._filesystem.join(root, 'apache', 'bin', 'httpd.exe')
        if self._filesystem.exists(path):
            return path
        _log.error('Could not find apache in the expected location. (path=%s)' % path)
        return None

    def _path_to_image_diff(self):
        base = os.environ.get('WEBKIT_IMAGE_DIFF')
        if base != None:
            return base

        base = spawn.find_executable('ImageDiff.exe')
        if base != None:
            return base

        return self._build_path('ImageDiff.exe')

    def _generate_all_test_configurations(self):
        configurations = []
        for build_type in self.ALL_BUILD_TYPES:
            configurations.append(TestConfiguration(version=self.version_name(), architecture='x86', build_type=build_type))
        return configurations

    def driver_name(self):
        if self.get_option('driver_name'):
            return self.get_option('driver_name')
        if self.get_option('webkit_test_runner'):
            return 'WebKitTestRunner.self'
        return 'DumpRenderTree.self'

    def _path_to_driver(self, configuration=None):
        """Returns the full path to the test driver (DumpRenderTree)."""
        return self._build_path(self.driver_name())

    def default_timeout_ms(self):
        return 90 * 1000

    def setup_environ_for_server(self, server_name=None):
        clean_env = super(PlayStationPort, self).setup_environ_for_server(server_name)

        # To run the C# dartapi app, we need TMP set correctly
        clean_env["TMP"] = os.environ["TMP"]
        return clean_env

    def _search_paths(self):
        search_paths = []
        # search_paths.append(self.port_name)
        search_paths.append(self._port_name)
        search_paths.append('wk2')
        search_paths.extend(self.get_option("additional_platform_directory", []))
        return search_paths

    def default_baseline_search_path(self, **kwargs):
        return list(map(self._webkit_baseline_path, self._search_paths()))

    def _port_specific_expectations_files(self, **kwargs):
        return [self._filesystem.join(self._webkit_baseline_path(p), 'TestExpectations') for p in reversed(self._search_paths())]

    def test_expectations_file_position(self):
        return 2

    # A device is the target host for a specific worker number.
    def target_host(self, worker_number=None):
        if worker_number is None:
            return self.host

        if not worker_number in self._device_for_worker_number_map:
            self._device_for_worker_number_map[worker_number] = PlayStationDevice(self, self.host, worker_number, self._port_name)
        return self._device_for_worker_number_map[worker_number]

    def setup_test_run(self, device_type=None):
        super(PlayStationPort, self).setup_test_run(device_type)

        for i in range(self.child_processes()):
            host = self.target_host(i)
            host.prepare_for_testing(self.ports_to_forward(), self.layout_tests_dir())

    def clean_up_test_run(self):
        super(PlayStationPort, self).clean_up_test_run()

        exception_list = []
        for i in range(self.child_processes()):
            device = self.target_host(i)
            try:
                if device:
                    device.finished_testing()
            except BaseException as e:
                trace = traceback.format_exc()
                if isinstance(e, Exception):
                    exception_list.append([e, trace])
                else:
                    exception_list.append([Exception('Exception tearing down {}'.format(device)), trace])
        if len(exception_list) == 1:
            raise
        elif len(exception_list) > 1:
            print('\n')
            for exception in exception_list:
                _log.error('{} raised: {}'.format(exception[0].__class__.__name__, exception[0]))
                _log.error(exception[1])
                _log.error('--------------------------------------------------')

            raise RuntimeError('Multiple failures when teardown devices')

    def child_processes(self):
        return int(self.get_option('child_processes'))

    def working_directory(self):
        return self._build_path()

    def create_driver(self, worker_number, no_timeout=False):
        """Return a newly created Driver subclass for starting/stopping the test driver."""
        return PlayStationDriverProxy(self, worker_number, self._driver_class(), pixel_tests=self.get_option('pixel_tests'), no_timeout=no_timeout)


class PlayStation4Port(PlayStationPort):
    port_name = "playstation-ps4"


class PlayStation5Port(PlayStationPort):
    port_name = "playstation-ps5"

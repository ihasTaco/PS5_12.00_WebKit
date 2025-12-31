import os
import time

from webkitpy.port.driver import Driver, DriverProxy


class PlayStationDriver(Driver):
    def run_test(self, driver_input, stop_when_done):
        # Workaround to stabilize test results
        time.sleep(0.4)

        return super(PlayStationDriver, self).run_test(driver_input, stop_when_done)

    def http_base_url(self, secure=None):
        scheme, port = ('https', 8443) if secure else ('http', 8000)
        host = os.getenv("LAYOUTTESTS_SERVER_IP")
        return "%s://%s:%d/" % (scheme, host, port)


class PlayStationDriverProxy(DriverProxy):
    def _make_driver(self, pixel_tests):
        self._driver_instance_constructor = PlayStationDriver
        return super(PlayStationDriverProxy, self)._make_driver(pixel_tests)

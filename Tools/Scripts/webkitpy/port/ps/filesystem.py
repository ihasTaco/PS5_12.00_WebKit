import logging
import os

from webkitpy.common.system.filesystem import FileSystem

try:
    from filesystem_imp import PlayStationFileSystemImp
except:

    class PlayStationFileSystemImp:
        @classmethod
        def map_base_host_path(cls, path):
            return None

_log = logging.getLogger(__name__)


class PlayStationFileSystem(FileSystem):
    def __init__(self):
        super(PlayStationFileSystem, self).__init__()
        self.sep = '/'

    def join(self, *comps):
        return self.sep.join(comps)

    def map_base_host_path(self, path):
        map_path = PlayStationFileSystemImp.map_base_host_path(path)
        if map_path is not None:
            return map_path
        return super(PlayStationFileSystem, self).map_base_host_path(path)

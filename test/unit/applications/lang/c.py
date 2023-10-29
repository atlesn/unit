from unit.applications.proto import ApplicationProto
from unit.option import option


class ApplicationC(ApplicationProto):
    def __init__(self, application_type='c'):
        self.application_type = application_type

    def load(self, prefix, name='c', **kwargs):

        self._load_conf(
	    {
                "listeners": {"*:7080": {"pass": "applications/x"}},
                "applications": {
                    "x": {
                        "type": self.get_application_type(),
                        "working_directory": f'{option.test_dir}/c',
                        "prefix": f"./{prefix}",
                        "cc": "/usr/bin/clang",
                        "flags": [
                            "-B/usr/bin",
                            "-I../../src",
                            "-I../../build/include",
                            "-Wall",
                            "-Wextra",
                            "-Werror"
                        ]
                   }
              }
            },
            **kwargs,
        )

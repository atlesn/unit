from unit.applications.proto import ApplicationProto
from unit.option import option


class ApplicationC(ApplicationProto):
    def __init__(self, application_type='c'):
        self.application_type = application_type

    def load(self, prefix, name='psgi', **kwargs):
        path = f'{option.test_dir}/c/{prefix}/'

        self._load_conf(
            {
                "listeners": {"*:7080": {"pass": f"applications/{prefix}"}},
                "applications": {
                    prefix: {
                        "type": self.get_application_type(),
                        "processes": {"spare": 0},
                        "working_directory": path,
                        "name": f'{name}'
                    }
                },
            },
            **kwargs,
        )

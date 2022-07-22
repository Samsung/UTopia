import json
from typing import List
from pathlib import Path


class Driver:
    """
    Used as a structure to store data for a driver.
    """

    def __init__(self, name: str, path: Path, source: Path):
        """
        Construct driver.

        :param name: driver name
        :param path: driver path
        :param source: driver main UT source path
        """
        self.name = name
        self.path = path
        self.source = source

    def __str__(self):
        """
        Print driver.

        :return: returns pretty print string for an object.
        """
        return (
            f"Driver[name: {self.name}\n"
            f"       path: {str(self.path)}\n"
            f"       source: {str(self.source)}]\n"
        )


class DriverParser:
    """
    Parse driver related information from a given path.
    """

    def __init__(self, basedir: Path):
        """
        Construct DriverParser

        :param basedir: The base directory to parse
        """

        name = basedir.name
        if not name.startswith("gen_"):
            raise RuntimeError("Unexpected Program State")

        self.basedir = basedir
        self.name = name[4:]

    def parse(self) -> List[Driver]:
        """
        Parse a given path to get driver information.

        :return: returns list of Drivers sorted with name.
        """
        drivers = self.load_drivers()
        report = self.load_gen_report()
        drivers = [
            Driver(driver_name, driver_path, report[driver_name])
            for driver_name, driver_path in drivers.items()
        ]
        drivers.sort(key=lambda x: x.name)
        return drivers

    def load_drivers(self) -> dict:
        """
        Parse a given path to initialize driver dictionary.

        :return: returns dictionary whose key is driver name and value is path
                 of its directory.
        """
        driver_paths = []
        for entry in self.basedir.iterdir():
            if entry.is_dir() and entry.name != "corpus":
                driver_paths.append(entry)

        return {path.name: path for path in driver_paths}

    def load_gen_report(self) -> dict:
        """
        Parse a given path to load fuzzgen report.

        :return: returns dictionary whose key is driver name and value for its
                 main UT source path.
        """
        report_path = self.basedir / "fuzzGen_Report.json"

        with open(report_path) as f:
            data = json.load(f)

        report = {
            ut_info["Name"]: Path(ut_info["FuzzTestSrc"])
            for ut_info in data["UT"]
        }
        return report

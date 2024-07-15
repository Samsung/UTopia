from helper.common.singleton import Singleton
import os


class Setting(metaclass=Singleton):
    output_dir: str = ""
    output_lib_dir: str = ""
    output_report_dir: str = ""
    output_report_api_path: str = ""
    output_code_dir: str = ""
    output_ast_dir: str = ""
    output_bc_dir: str = ""
    output_report_build_db_dir: str = ""
    target_analyzer_report_dir_path: str = ""
    ut_analyzer_report_dir_path: str = ""
    fuzz_generator_report_path: str = ""

    def set_output_dir(self, output_dir: str) -> None:
        self.output_dir = os.path.join(output_dir)
        self.output_fuzzer_dir = os.path.join(output_dir, "fuzzers")
        self.output_lib_dir = os.path.join(output_dir, "libs")
        self.output_report_dir = os.path.join(output_dir, "reports")
        self.output_report_api_path = os.path.join(self.output_report_dir, "api.json")
        self.output_report_build_db_dir = os.path.join(
            self.output_report_dir, "build_db"
        )
        self.output_code_dir = os.path.join(output_dir, "code")
        self.output_ast_dir = os.path.join(self.output_code_dir, "ast")
        self.output_bc_dir = os.path.join(self.output_code_dir, "bc")
        self.target_analyzer_report_dir_path = os.path.join(
            self.output_report_dir, "target_analyzer"
        )
        self.ut_analyzer_report_dir_path = os.path.join(
            self.output_report_dir, "ut_analyzer"
        )
        self.fuzzer_generator_report_path = os.path.join(
            self.output_report_dir, "fuzz_generator.json"
        )

    def __str__(self) -> str:
        return "\n".join(
            [
                "Path:",
                f"Output Directory: {self.output_dir}",
            ]
        )

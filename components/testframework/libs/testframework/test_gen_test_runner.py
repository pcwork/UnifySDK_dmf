#!/usr/bin/env python3

import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).with_name("gen_test_runner.py")


class GenerateUnityRunnerTest(unittest.TestCase):
    def test_writes_runner_to_output_file(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            test_file = temp_path / "sample_test.c"
            output_file = temp_path / "sample_test_runner.c"

            test_file.write_text(
                textwrap.dedent(
                    """\
                    #include "unity.h"

                    void test_sample_behavior(void)
                    {
                    }
                    """
                ),
                encoding="utf-8",
            )

            result = subprocess.run(
                [sys.executable, str(SCRIPT_PATH), str(test_file), str(output_file)],
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(output_file.exists())

            runner_text = output_file.read_text(encoding="utf-8")
            self.assertIn("int main(int argc, char** argp)", runner_text)
            self.assertIn("void test_sample_behavior();", runner_text)
            self.assertIn('UnityDefaultTestRun(&test_sample_behavior, "test_sample_behavior"', runner_text)


if __name__ == "__main__":
    unittest.main()

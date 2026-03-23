import subprocess
import tempfile
import unittest
from pathlib import Path


class TestPreprocessScripts(unittest.TestCase):
    def test_generate_cbt_edges(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "cbt_edges.txt"
            subprocess.run(
                ["uv", "run", "python", "scripts/preprocess/generate_cbt.py", "--depth", "2", "--output", str(out)],
                check=True,
            )
            lines = out.read_text(encoding="utf-8").strip().splitlines()
            self.assertTrue(lines[0].startswith("# Complete Binary Tree"))
            # depth=2 => 7 nodes => 6 edges
            self.assertEqual(len(lines), 7)
            self.assertIn("0 1", lines)
            self.assertIn("0 2", lines)

    def test_generate_cbt_nodes(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "cbt_nodes.txt"
            subprocess.run(
                ["uv", "run", "python", "scripts/preprocess/generate_cbt.py", "--nodes", "10", "--output", str(out)],
                check=True,
            )
            lines = out.read_text(encoding="utf-8").strip().splitlines()
            self.assertTrue(lines[0].startswith("# Complete Binary Tree with 10 nodes"))
            self.assertEqual(len(lines), 10)
            self.assertIn("0 1", lines)
            self.assertIn("4 9", lines)

    def test_generate_er_seeded_deterministic(self):
        with tempfile.TemporaryDirectory() as tmp:
            out1 = Path(tmp) / "er1.txt"
            out2 = Path(tmp) / "er2.txt"
            cmd = [
                "uv",
                "run",
                "python",
                "scripts/preprocess/generate_er.py",
                "--nodes",
                "12",
                "--prob",
                "0.25",
                "--seed",
                "1337",
            ]
            subprocess.run([*cmd, "--output", str(out1)], check=True)
            subprocess.run([*cmd, "--output", str(out2)], check=True)
            self.assertEqual(
                out1.read_text(encoding="utf-8"),
                out2.read_text(encoding="utf-8"),
            )

    def test_generate_er_invalid_probability_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "bad_er.txt"
            proc = subprocess.run(
                [
                    "uv",
                    "run",
                    "python",
                    "scripts/preprocess/generate_er.py",
                    "--nodes",
                    "12",
                    "--prob",
                    "1.2",
                    "--output",
                    str(out),
                ],
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(proc.returncode, 0)


if __name__ == "__main__":
    unittest.main()

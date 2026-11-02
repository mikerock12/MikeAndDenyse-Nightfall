"""Compare unchanged C# physics/levels (MathF adapter) against the native core.

Not a claim about Unity rendering, Unity RNG, or on-device performance.
Run after building Saved/ReferenceRunner and NightfallParityTrace.
"""
import csv
import io
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
native = root / 'build/windows-msvc-release/Release/NightfallParityTrace.exe'
reference = root / 'Saved/ReferenceRunner/NightfallReference.dll'
def rows(command):
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    return list(csv.reader(io.StringIO(result.stdout)))
expected = rows(['dotnet', str(reference)])
actual = rows([str(native)])
failures = []
max_error = 0.0
if len(expected) != len(actual):
    failures.append({'lengths': [len(expected), len(actual)]})
for index, (want, got) in enumerate(zip(expected, actual)):
    for field, (left, right) in enumerate(zip(want, got)):
        try:
            difference = abs(float(left) - float(right))
            max_error = max(max_error, difference)
            floating = (want[0] == 'STEP' and field in [5,6,7,8,12,13,14]) or (want[0] in ['COMBAT','BOSS'] and field in [4,5,7,8])
            same = difference <= (0.001 if floating else 0.0)
        except ValueError:
            same = left == right
        if not same:
            if len(failures) < 20:
                failures.append({'row': index, 'case': want[:5], 'field': field, 'csharp': left, 'cpp': right})
            break
summary = {'reference': 'unchanged Sim.cs + LevelBuilder.cs + GameData.cs / System.MathF adapter', 'steps': sum(row[0] == 'STEP' for row in expected), 'combat_steps': sum(row[0] == 'COMBAT' for row in expected), 'boss_steps': sum(row[0] == 'BOSS' for row in expected), 'levels': sum(row[0] == 'LEVEL' for row in expected), 'rng': 'controlled xorshift32 adapter, not UnityEngine.Random equivalence', 'max_numeric_error': max_error, 'passed': not failures, 'first_mismatches': failures}
output = root / 'Saved/reference-parity-report.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(summary, indent=2), encoding='utf-8')
print(json.dumps(summary, indent=2))
sys.exit(0 if not failures else 1)

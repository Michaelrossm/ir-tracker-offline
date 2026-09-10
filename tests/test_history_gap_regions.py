from __future__ import annotations

import shutil
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which("node"), "Node.js is required for browser helper tests")
class HistoryGapRegionTests(unittest.TestCase):
    def test_gap_regions_cover_only_missing_time(self) -> None:
        script = r'''
global.window = global;
global.location = {href:'http://localhost/',protocol:'http:',host:'localhost',pathname:'/',search:'',hash:''};
global.history = {replaceState(){}};
global.localStorage = {getItem(){return null},setItem(){}};
global.fetch = async()=>({});
global.document = {readyState:'loading',documentElement:{style:{setProperty(){}}},addEventListener(){}};
const fs = require('fs');
eval(fs.readFileSync(process.argv[1], 'utf8'));
const equal = (actual, expected, label) => {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) throw new Error(label + ': ' + JSON.stringify(actual));
};
const regions = (values, from, to, step, now) => window.irGapRegions(values, from, to, step, now);
const refine = (coarse, precise, from) => window.irRefineGapRegions(coarse, precise, from);
equal(regions([{ts:0},{ts:60},{ts:300}], 0, 360, 60, 360), [{start:120,end:300}], 'single');
equal(regions([{ts:0},{ts:60},{ts:300},{ts:600}], 0, 660, 60, 660), [{start:120,end:300},{start:360,end:600}], 'multiple');
equal(regions([{ts:0},{ts:60},{ts:120}], 0, 180, 60, 180), [], 'no gap');
equal(regions([{ts:180}], 0, 240, 60, 240), [{start:0,end:180}], 'leading');
equal(regions([{ts:0},{ts:60}], 0, 300, 60, 300), [{start:120,end:300}], 'trailing');
equal(regions([], 0, 300, 60, 300), [{start:0,end:300}], 'empty');
equal(refine([{start:42000,end:43200},{start:48600,end:50400}],
             [{start:43080,end:43200},{start:49920,end:50040}], 42300),
      [{start:42000,end:42300},{start:43080,end:43200},{start:49920,end:50040}],
      'minute precision replaces coarse regions');
equal(refine([{start:120,end:300}], [], null), [{start:120,end:300}],
      'missing precision keeps coarse regions');
const dashboard=fs.readFileSync(process.argv[2], 'utf8');
const dRawStep=60;
eval(dashboard.split('\n').find(line=>line.startsWith('function dAggregate(')));
const raw=[0,60,240,300,360].map(ts=>({ts,avg:100,min:100,max:100}));
const points=dAggregate(raw,900);
equal(points.map(p=>[p.ts,p.end,p.breakBefore]),[[0,120,false],[240,420,true]],
      'two-minute outage splits a single display bucket without moving timestamps');
equal(regions(raw,0,420,60,420),[{start:120,end:240}], 'exact missing two minutes');
'''
        result = subprocess.run(
            ["node", "-e", script, str(ROOT / "web" / "common.js"),
             str(ROOT / "web" / "dashboard.js")],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_canvas_uses_transparent_regions_instead_of_gap_markers(self) -> None:
        source = (ROOT / "web" / "dashboard.js").read_text(encoding="utf-8")
        self.assertIn("function dDrawGapRegions", source)
        self.assertIn("ctx.fillRect(start,top,width,bottom-top)", source)
        self.assertIn("if(width>=72)", source)
        self.assertNotIn("function dDrawGaps", source)

    def test_history_api_streams_minute_precise_gap_metadata(self) -> None:
        source = (ROOT / "src/app/web/DashboardHistory.cpp").read_text(
            encoding="utf-8")
        self.assertIn("void streamMinuteGapDetails", source)
        self.assertIn('range == "day" || range == "two_days"', source)
        self.assertIn("gap_precision_from", source)
        self.assertIn("previousTimestamp + 60U", source)
        self.assertIn("record.timestamp - previousTimestamp > 90U", source)


if __name__ == "__main__":
    unittest.main()

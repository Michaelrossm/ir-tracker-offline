#!/usr/bin/env python3
from enum import IntEnum
import unittest
from pathlib import Path

class R(IntEnum):
    POWERON=1
    SW=2
    PANIC=3
    WDT=4
    TASK=5
    INT=6
    BROWNOUT=7

CRASH={R.PANIC,R.WDT,R.TASK,R.INT,R.BROWNOUT}

def next_count(count, healthy, reason):
    if reason in CRASH:
        return count if healthy else min(255, count+1)
    return 0

def main():
    c=0
    for _ in range(3):
        c=next_count(c,False,R.WDT)
    assert c==3
    assert next_count(2,False,R.SW)==0
    assert next_count(2,True,R.WDT)==2
    baseline=100
    assert (101-baseline)<2
    assert (102-baseline)>=2
    print("professional policy tests: OK")

class ProfessionalPolicyTests(unittest.TestCase):
    def test_package_policy(self):
        main()

    def test_integration_gates(self):
        root = Path(__file__).resolve().parents[1]
        main_src = (root / "src/main.cpp").read_text(encoding="utf-8")
        self.assertIn("if (productRuntimeAllowsHistoryMigration() &&", main_src)
        ota = (root / "src/app/update/OtaManager.cpp").read_text(encoding="utf-8")
        self.assertIn("if (!productRuntimeAllowsAutomaticUpdate() ||", ota)
        web = (root / "src/app/web/WebApi.cpp").read_text(encoding="utf-8")
        self.assertIn('"/api/v1/update/bundle"', web)
        for module in ("core/ProductSafety", "core/ProductRuntime", "meter/MeterAutoCommissioning",
                       "diagnostics/ProductExperience", "diagnostics/FactoryNvsProbe"):
            text = (root / ("src/app/" + module + ".cpp")).read_text(encoding="utf-8")
            for forbidden in ("LittleFS.format(", "prefs.clear(", "history.clear(", "esp_partition_erase_range("):
                self.assertNotIn(forbidden, text)
        auto = (root / "src/app/meter/MeterAutoCommissioning.cpp").read_text(encoding="utf-8")
        self.assertEqual(auto.count("saveConfig();"), 1)

if __name__=="__main__":
    unittest.main()

import math
import unittest


def range_from_ring(slots, count, write_index, since, until):
    """Model HistoryStore's logical lower-bound and sequential ring read."""
    if not count or since > until:
        return [], 0
    capacity = len(slots)
    first = 0 if count < capacity else write_index
    reads = 0

    def logical(index):
        nonlocal reads
        reads += 1
        return slots[(first + index) % capacity]

    low, high = 0, count
    while low < high:
        middle = low + (high - low) // 2
        record = logical(middle)
        if record < since:
            low = middle + 1
        else:
            high = middle

    result = []
    for index in range(low, count):
        record = logical(index)
        if record > until:
            break
        if record >= since:
            result.append(record)
    return result, reads


def make_ring(records, capacity, write_index=0):
    slots = [None] * capacity
    if len(records) < capacity:
        slots[: len(records)] = records
        return slots, len(records), len(records)
    for logical, record in enumerate(records[-capacity:]):
        slots[(write_index + logical) % capacity] = record
    return slots, capacity, write_index


class HistoryRangeSearchTests(unittest.TestCase):
    def assert_range(self, records, capacity, write_index, since, until):
        slots, count, actual_write = make_ring(records, capacity, write_index)
        result, reads = range_from_ring(
            slots, count, actual_write, since, until
        )
        expected = [value for value in records[-capacity:] if since <= value <= until]
        self.assertEqual(expected, result)
        if count:
            self.assertLessEqual(
                reads,
                math.ceil(math.log2(count)) + len(expected) + 2,
            )
        return reads, len(result)

    def test_not_full_today_yesterday_and_old_day(self):
        records = list(range(1_700_000_000, 1_700_172_800, 60))
        self.assert_range(records, 4000, 0, records[-720], records[-1])
        self.assert_range(records, 4000, 0, records[-2160], records[-721])
        self.assert_range(records, 4000, 0, records[60], records[1499])

    def test_full_ring_without_physical_wrap(self):
        records = list(range(1_700_000_000, 1_700_000_000 + 64 * 60, 60))
        self.assert_range(records, 64, 0, records[10], records[30])

    def test_full_wrapped_ring(self):
        records = list(range(1_700_000_000, 1_700_000_000 + 64 * 60, 60))
        self.assert_range(records, 64, 23, records[50], records[63])

    def test_empty_and_outside_ranges(self):
        self.assertEqual(([], 0), range_from_ring([None] * 16, 0, 0, 0, 10))
        records = list(range(1_700_000_000, 1_700_003_600, 60))
        self.assert_range(records, 128, 0, records[-1] + 60, records[-1] + 600)
        self.assert_range(records, 128, 0, records[0] - 600, records[0] - 60)

    def test_since_before_oldest_until_after_newest(self):
        records = list(range(1_700_000_000, 1_700_003_600, 60))
        self.assert_range(records, 128, 0, records[0] - 60, records[-1] + 60)

    def test_week_month_year_resolutions(self):
        for step, count, selected in (
            (900, 31 * 24 * 4, 7 * 24 * 4),
            (900, 180 * 24 * 4, 31 * 24 * 4),
            (86400, 730, 365),
        ):
            records = [1_700_000_000 + index * step for index in range(count)]
            self.assert_range(
                records,
                count + 32,
                0,
                records[-selected],
                records[-1],
            )


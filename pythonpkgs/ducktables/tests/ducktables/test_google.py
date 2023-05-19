
from unittest import TestCase
from ducktables import google


class TestGoogle(TestCase):
    def test_sheet(self):
        self.assertTrue(hasattr(google, 'sheet'))


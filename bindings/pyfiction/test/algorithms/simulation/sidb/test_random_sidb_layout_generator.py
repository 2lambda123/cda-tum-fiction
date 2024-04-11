from mnt.pyfiction import *
import unittest
import os


class TestRandomSiDBLayoutGenerator(unittest.TestCase):

    def test_area_with_one_coordinate_100_lattice(self):
        params = generate_random_sidb_layout_params()
        params.number_of_sidbs = 1
        params.coordinate_pair = ((10,10), (10,10))
        result_lyt = generate_random_sidb_layout_100(sidb_lattice_100(), params)
        self.assertEqual(result_lyt.num_cells(), 1)
        cell = (result_lyt.cells())[0]
        self.assertEqual(cell.x, 10)
        self.assertEqual(cell.y, 10)

    def test_area_with_one_coordinate_111_lattice(self):
        params = generate_random_sidb_layout_params()
        params.number_of_sidbs = 1
        params.coordinate_pair = ((10, 10), (10, 10))
        result_lyt = generate_random_sidb_layout_111(sidb_lattice_111(), params)
        self.assertEqual(result_lyt.num_cells(), 1)
        cell = (result_lyt.cells())[0]
        self.assertEqual(cell.x, 10)
        self.assertEqual(cell.y, 10)

if __name__ == '__main__':
    unittest.main()

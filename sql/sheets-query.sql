
SELECT *
FROM python_table('google_', 'sheet',
     { 'name': 'VARCHAR', 'gender': 'VARCHAR', 'class_level': 'VARCHAR', 'state': 'VARCHAR', 'major': 'VARCHAR', 'extracurricular': 'VARCHAR'},
     ['1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms', 'Class Data!A2:F31']);

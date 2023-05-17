    SELECT *
    FROM pytable('google_:sheet', '1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms', 'Class Data!A2:F31',
         columns = {
             'name': 'VARCHAR', 'gender': 'VARCHAR', 'class_level': 'VARCHAR',
             'state': 'VARCHAR', 'major': 'VARCHAR', 'extracurricular': 'VARCHAR'}
    );

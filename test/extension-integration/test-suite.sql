
load python_udf;

-- SELECT *
-- FROM pytable('ducktables.google:sheet', '1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms', 'Class Data!A2:F31',
--      columns = {
--              'name': 'VARCHAR', 'gender': 'VARCHAR', 'class_level': 'VARCHAR',
--              'state': 'VARCHAR', 'major': 'VARCHAR', 'extracurricular': 'VARCHAR'}
--     )
-- ORDER BY name ASC
-- LIMIT 2;

SELECT *
FROM pytable('ducktables.github:repos_for', 'markroddy',
     columns = {'repo': 'VARCHAR', 'description': 'VARCHAR', 'language': 'VARCHAR'})
WHERE repo like '%duck%';

# Note this file is named `google_` instead of `google` to prevent name conflicts
# with official google libraries that would prevent this script from being importable.

import os, sys
import gspread
from oauth2client.service_account import ServiceAccountCredentials
from datetime import datetime
from googleapiclient.discovery import build
from google.oauth2 import service_account

def _key_path(key_file_path = None):
    if not key_file_path:
        key_file_path = os.environ.get('GOOGLE_APPLICATION_CREDENTIALS')
        if not key_file_path:
            raise Exception("No key file specified, and no environment variable found")
    return key_file_path
    

def analytics(view_id, start_date, end_date, dimensions = None, metrics = None, key_file_path = None):
    """
    SQL Usage:
    SELECT * FROM pytable('google_:analytics', '<view-id>', '<start-date>', '<end-date>',
      columns = {
        -- Dimensions
        'date': 'VARCHAR', 'hour': 'VARCHAR', 'pagePath': 'VARCHAR', 'source': 'VARCHAR', 'medium': 'VARCHAR',
        -- Metrics
        'sessions': 'INT', 'users': 'INT', 'pageviews': 'INT', 'avgSessionDuration': 'FLOAT', 'bounceRate': 'FLOAT'
        });

    # Note that this function lets you specify a list of dimensions and/or metrics. If you use non-default
    # values for these you will need to update the 'columns' struct from the example above.
    """
    key_file_path = _key_path(key_file_path)
    # Load credentials from JSON file
    credentials = service_account.Credentials.from_service_account_file(key_file_path)
    
    # Build the Google Analytics Reporting API client
    analytics = build('analyticsreporting', 'v4', credentials=credentials)

    if not metrics:
        metrics = ['ga:sessions', 'ga:users', 'ga:pageviews', 'ga:avgSessionDuration', 'ga:bounceRate']
    if not dimensions:
        dimensions = ['ga:date', 'ga:hour', 'ga:pagePath', 'ga:source', 'ga:medium']
    
    # Retrieve GA data using the API
    response = analytics.reports().batchGet(
        body={
            'reportRequests': [{
                'viewId': view_id,
                'dateRanges': [{'startDate': start_date, 'endDate': end_date}],
                # 'metrics': [{'expression': 'ga:sessions'}, {'expression': 'ga:users'}],
                'metrics': [ {'expression': m} for m in metrics],
                # 'dimensions': [{'name': 'ga:date'}]
                'dimensions': [{'name': d} for d in dimensions],
            }]
        }
    ).execute()
    
    # Extract data from the API response
    report = response['reports'][0]
    for row in report['data']['rows']:
        yield row['dimensions'] + row['metrics'][0]['values']

def sheet(sheets_url_or_key, cell_range, key_file_path = None):
    """
    SQL Usage:
    SELECT *
    FROM pytable('google_:sheet', '1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms', 'Class Data!A2:F31',
         columns = {
             'name': 'VARCHAR', 'gender': 'VARCHAR', 'class_level': 'VARCHAR',
             'state': 'VARCHAR', 'major': 'VARCHAR', 'extracurricular': 'VARCHAR'}
    );
    """
    key_file_path = _key_path(key_file_path)

    # Support specifying the full ULR, or the shorter 'key' identierifer. We generate
    # the full url if the later is supplied.
    if sheets_url_or_key.startswith('https://docs.google.com/spreadsheets/d/'):
        sheets_url = sheets_url_or_key
    else:
        sheets_url = "https://docs.google.com/spreadsheets/d/" + sheets_url_or_key + "/edit#gid=0"
        
    # authorize access to the Google Sheets API
    scope = ['https://spreadsheets.google.com/feeds',
             'https://www.googleapis.com/auth/drive']
    creds = ServiceAccountCredentials.from_json_keyfile_name(key_file_path, scope)
    client = gspread.authorize(creds)

    # extract the worksheet name and range from the cell range string
    worksheet_name, cell_range = cell_range.split('!')
    worksheet = client.open_by_url(sheets_url).worksheet(worksheet_name)

    # get the values of the specified cells and convert to Python types
    cell_list = worksheet.range(cell_range)
    # rows = []
    current_row = []
    for cell in cell_list:
        if cell.value.isdigit():
            current_row.append(int(cell.value))
        elif cell.value.replace('.', '', 1).isdigit():
            current_row.append(float(cell.value))
        elif cell.value.startswith('='):
            # evaluate formulas and include result in output
            formula_value = worksheet.cell(cell.row, cell.col).value
            try:
                formula_result = worksheet.cell(cell.row, cell.col).value
                if isinstance(formula_result, (int, float)):
                    current_row.append(formula_result)
                elif isinstance(formula_result, datetime):
                    current_row.append(formula_result)
                else:
                    current_row.append(formula_value)
            except:
                current_row.append(formula_value)
        else:
            try:
                # try to parse date and time values
                date_time = datetime.strptime(cell.value, '%m/%d/%Y %H:%M:%S')
                current_row.append(date_time)
            except ValueError:
                current_row.append(cell.value)
        # check if we've reached the end of a row
        if cell.col == ord('F') - ord('A') + 1:
            yield current_row
            current_row = []
    # return rows

if __name__ == '__main__':
    sheet_url = "https://docs.google.com/spreadsheets/d/1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms/edit#gid=0"
    cell_range = "Class Data!A2:F31"
    rows = sheet(sheet_url, cell_range)
    # for r in rows:
    #     print(r)

    view_id = '...........'
    start_date = '2023-01-01'
    end_date = '2023-03-31'

    data_iterator = analytics(view_id, start_date, end_date)

    # Iterate over the data
    for record in data_iterator:
        print(record)


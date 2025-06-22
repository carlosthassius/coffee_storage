import json
from decimal import Decimal
from boto3 import resource

class DecimalEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, Decimal):
            # converte para float (ou int(o) se for sempre inteiro)
            return float(o)
        return super().default(o)

dynamodb = resource('dynamodb')
table = dynamodb.Table('iot_db')

def lambda_handler(event, context):
    try:
        response = table.scan()
        items = response.get('Items', [])

        filtered_sorted = sorted(
            [item for item in items if 'ts' in item],
            key=lambda x: int(x['ts']),
            reverse=True
        )

        return {
            "statusCode": 200,
            "headers": {
                "Access-Control-Allow-Origin": "*",
                "Content-Type": "application/json"
            },
            "body": json.dumps(filtered_sorted, cls=DecimalEncoder)
        }

    except Exception as e:
        print("Error scanning DynamoDB:", e)
        return {
            "statusCode": 500,
            "headers": {
                "Access-Control-Allow-Origin": "*",
                "Content-Type": "application/json"
            },
            "body": json.dumps({"error": str(e)})
        }

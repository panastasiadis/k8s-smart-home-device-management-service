# management/commands/check_actions.py
import os
import time

from devices.models import Action, Sensor
from devices.services import send_command_to_sensor
from django.core.management.base import BaseCommand
from influxdb_client import InfluxDBClient


def is_condition_met(comparison_type, value, check_value):
    if comparison_type == '>':
        return value > check_value
    elif comparison_type == '<':
        return value < check_value
    elif comparison_type == '=':
        return value == check_value
    else:
        raise ValueError('Invalid comparison type')


class Command(BaseCommand):
    help = 'Check actions and perform tasks if conditions are met'

    def handle(self, *args, **kwargs):

        ENV_INFLUXDB_URL = f'{os.getenv("DOCKER_INFLUXDB_HOST_TYPE")}://{os.getenv("DOCKER_INFLUXDB_HOST")}:{os.getenv("DOCKER_INFLUXDB_PORT")}'
        ENV_INFLUXDB_TOKEN = os.getenv('DOCKER_INFLUXDB_INIT_ADMIN_TOKEN')
        ENV_INFLUXDB_ORG = os.getenv('DOCKER_INFLUXDB_INIT_ORG')
        ENV_INFLUXDB_BUCKET = os.getenv('DOCKER_INFLUXDB_INIT_BUCKET')

        influx_client = InfluxDBClient(
            url=ENV_INFLUXDB_URL, token=ENV_INFLUXDB_TOKEN, org=ENV_INFLUXDB_ORG)

        while True:
            actions = Action.objects.all()

            for action in actions:
                if action.active:
                    initiator_sensor = action.initiator_sensor

                    # Construct Flux query
                    query = (f'from(bucket: "{ENV_INFLUXDB_BUCKET}") '
                            f'|> range(start: -10s) '
                            '|> timeShift(duration: 3h) '
                            f'|> filter(fn: (r) => r["_measurement"] == "sensor_data") '
                            f'|> filter(fn: (r) => r["sensor_serial"] == "{initiator_sensor.serial}") '
                            '|> last()')

                    # Execute Flux query
                    query_api = influx_client.query_api()
                    tables = query_api.query(query)
                    for table in tables:
                        for row in table.records:
                            last_value = row.values.get('_value')

                            print('Last: ', last_value)
                            print('Threshold: ', action.check_value)
                            
                            print(action.recipient_sensor.device.serial)
                            print(action.recipient_sensor.serial)
                            if is_condition_met(action.comparison_type, last_value, action.check_value):
                                
                                print('Actions Manager: Condition is met! Sending command to sensor..')
                                
                                send_command_to_sensor(
                                    command=action.command, 
                                    device_serial=action.recipient_sensor.device.serial, 
                                    sensor_serial=action.recipient_sensor.serial)

                                action.active = False
                                action.save()
            time.sleep(3)  # Wait for 60 seconds before checking again

import serial
from rest_framework import filters, status, viewsets
from rest_framework.response import Response
from rest_framework.views import APIView

from .exceptions import (DeviceGenericException, DeviceNoResponseException,
                         DeviceNotFoundException, DeviceUnknownSerialException)
from .models import Device, Room, Sensor
from .serializers import (CommandSerializer, DeviceSerializer,
                          FlashSerialDeviceSerializer, RoomSerializer,
                          SensorSerializer)
from .services import send_command_to_sensor
from .utils import compile_and_flash_device, parse_device_serial


class RoomsViewSet(viewsets.ModelViewSet):
    queryset = Room.objects.all()
    serializer_class = RoomSerializer
    filter_backends = [filters.OrderingFilter]
    ordering = ['pk']


class DeviceViewSet(viewsets.ModelViewSet):
    http_method_names = ['get', 'head', 'put', 'patch']

    queryset = Device.objects.all()
    serializer_class = DeviceSerializer
    filter_backends = [filters.OrderingFilter]
    ordering = ['pk']



class SensorViewSet(viewsets.ModelViewSet):
    http_method_names = ['get', 'head', 'put', 'patch']

    queryset = Sensor.objects.all()
    serializer_class = SensorSerializer
    filter_backends = [filters.OrderingFilter]
    ordering = ['pk']


class FlashSerialDevice(APIView):
    '''API endpoint for flashing an Arduino device over a serial connection.'''

    def post(self, request, id):

        serializer = FlashSerialDeviceSerializer(data=request.data)

        if serializer.is_valid():

            try:
                # Retrieve the Device instance based on the provided id
                device_instance = Device.objects.get(pk=id)

                # Extract validated data from the serializer
                validated_data = serializer.validated_data
                ssid_name = validated_data.get('ssid_name', {})
                ssid_password = validated_data.get('ssid_password', {})
                ip_address = validated_data.get('ip_address', {})

                # Get necessary information for compilation and flash process from the Device instance
                fqbn = device_instance.fqbn
                sketch_name = device_instance.sketch_name

                # Call the utility function to compile and flash the device
                compile_and_flash_device(ssid_name,
                                         ssid_password,
                                         ip_address,
                                         fqbn,
                                         sketch_name)

                # Return success response
                return Response({'success': f'Device {device_instance.serial} was flashed successfuly.'})

            except Device.DoesNotExist:
                # Handle the case where the device is not found
                return Response({'error': f'Device with ID {id} not found.'})

            except Exception as e:
                # Handle other exceptions and return error response
                return Response({'error': str(e)}, status=status.HTTP_400_BAD_REQUEST)
        else:
            # If validation fails, return the DRF validation errors
            return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)


class GetSerialDevice(APIView):
    '''API view for retrieving device information based on serial communication.'''

    def get(self, request, *args, **kwargs):
        try:
            try:
                # Establish a serial connection and read data
                ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=25)
                data = ser.readline().decode('utf-8').strip()

                # Raise exception if no data is received
                if not data:
                    raise DeviceNoResponseException

                # Parse device serial and fetch corresponding device information
                device_serial = parse_device_serial(data)
                device = Device.objects.get(serial=device_serial)
                serializer = DeviceSerializer(device)

                return Response(serializer.data)

            # Raise exception if the device with the parsed serial is not found in the database
            except Device.DoesNotExist:
                raise DeviceUnknownSerialException

            # Raise exception if there is an issue with the serial connection
            except serial.serialutil.SerialException:
                raise DeviceNotFoundException

        except DeviceGenericException as e:
            return Response({'error': e.message})


class SendCommandToSensorView(APIView):
    '''API view for sending a command to a sensor.'''
    def post(self, request):
        serializer = CommandSerializer(data=request.data)

        if serializer.is_valid():

            sensor_serial = serializer.validated_data.get('sensor').serial
            device_serial = serializer.validated_data.get('device').serial
            command = serializer.validated_data.get('command')

            send_command_to_sensor(device_serial, sensor_serial, command)
            return Response({"detail": "Command was sent successfully"})

        return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)

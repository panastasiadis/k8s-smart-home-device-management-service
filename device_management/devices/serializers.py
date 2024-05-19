from rest_framework import serializers

from .models import Action, Device, Room, Sensor


class RoomSerializer(serializers.ModelSerializer):
    class Meta:
        model = Room
        fields = '__all__'


class DeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = Device
        fields = ('id', 'name', 'serial', 'model', 'room', 'status')
        read_only_fields = ('serial', 'model', 'status')


class SensorSerializer(serializers.ModelSerializer):
    device = DeviceSerializer(read_only=True)

    class Meta:
        model = Sensor
        depth = 1
        fields = '__all__'
        read_only_fields = ('serial', 'model', 'accepts_commands', 'device')


class FlashSerialDeviceSerializer(serializers.Serializer):
    ssid_name = serializers.CharField(write_only=True)
    ssid_password = serializers.CharField(write_only=True)
    ip_address = serializers.IPAddressField(write_only=True)

    class Meta:
        model = Device
        fields = '__all__'


class CommandSerializer(serializers.Serializer):
    sensor_id = serializers.IntegerField()
    command = serializers.CharField()

    def validate(self, data):
        data = super().validate(data)

        # Extract sensor ID and command
        sensor_id = data.get('sensor_id', None)
        command = data.get('command', None)

        # Check if the sensor with the given ID exists
        sensor = Sensor.objects.filter(pk=sensor_id).first()

        if not sensor:
            raise serializers.ValidationError(
                "Sensor with this ID does not exist.")

        # Retrieve the device associated with the sensor
        device = Device.objects.filter(pk=sensor.device_id).first()

        if not sensor.accepts_commands:
            raise serializers.ValidationError(
                "This sensor does not accept commands.")

        if device.status == 'disconnected':
            raise serializers.ValidationError(
                "Sending command to a disconnected device.")

        if device.status == 'not_set_up':
            raise serializers.ValidationError(
                "Sending command to a device that is not set up.")

        if sensor.available_commands is not None and command not in sensor.available_commands:
            raise serializers.ValidationError(
                "Invalid command for this sensor.")

        # Attach the sensor and device objects to the serializer data
        data['sensor'] = sensor
        data['device'] = device

        return data


class ActionSerializer(serializers.ModelSerializer):
    class Meta:
        model = Action
        fields = '__all__'

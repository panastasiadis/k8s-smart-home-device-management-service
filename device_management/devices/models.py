from django.db import models


class Room(models.Model):
    name = models.CharField(max_length=255, unique=True)
    description = models.TextField(blank=True)

    def __str__(self):
        return self.name


class Device(models.Model):
    serial = models.CharField(max_length=255, unique=True)
    name = models.CharField(max_length=255, null=True, blank=True)
    model = models.CharField(max_length=255)
    fqbn = models.CharField(max_length=255)
    sketch_name = models.CharField(max_length=255)
    status = models.CharField(max_length=20, choices=[('connected', 'Connected'), (
        'disconnected', 'Disconnected'), ('not_set_up', 'Not Set Up'),], default='not_set_up')
    room = models.ForeignKey(
        Room, on_delete=models.SET_NULL, null=True, blank=True, related_name='devices')

    def __str__(self):
        return self.model + ': ' + self.serial


class Sensor(models.Model):
    serial = models.CharField(max_length=255, unique=True)
    name = models.CharField(max_length=255, null=True, blank=True)
    model = models.CharField(max_length=255)
    type = models.CharField(max_length=255)
    description = models.TextField()
    accepts_commands = models.BooleanField(default=False)
    available_commands = models.JSONField(null=True, blank=True)
    device = models.ForeignKey(
        Device, on_delete=models.CASCADE, related_name='sensors')

    def __str__(self):
        return self.model + ': ' + self.serial

from django.contrib import admin

from .models import Action, Device, Room, Sensor

# Register your models here.
admin.site.register(Device)
admin.site.register(Sensor)
admin.site.register(Room)
admin.site.register(Action)

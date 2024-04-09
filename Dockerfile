# syntax=docker/dockerfile:1
FROM python:3
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

# install deps and necessary libs for arduino cli integration
RUN apt-get update && apt-get install -y curl && \ 
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/bin sh && \
    arduino-cli board list && \
    arduino-cli core update-index
WORKDIR /opt/app/
RUN echo 'board_manager:' > /opt/app/cli-config.yml && \
    echo '  additional_urls:' >> /opt/app/cli-config.yml && \
    echo '    - http://arduino.esp8266.com/stable/package_esp8266com_index.json' >> /opt/app/cli-config.yml && \
    arduino-cli core install esp8266:esp8266 --config-file /opt/app/cli-config.yml && \
    arduino-cli core install arduino:samd && \
    arduino-cli lib install "DHT sensor library" && \
    arduino-cli lib install BH1750 && \
    arduino-cli lib install PubSubClient && \
    arduino-cli lib install ArduinoJson

COPY requirements.txt requirements.txt
RUN pip install --no-cache-dir -r requirements.txt

WORKDIR /code/device_management
COPY device_management .

CMD ["./run.sh"]
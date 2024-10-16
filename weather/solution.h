#pragma once

#include "printer.h"

#include <memory>
#include <string>
#include <map>
#include <type_traits>
#include <stdexcept>

template<class T>
concept Sensor = std::is_default_constructible_v<T> && requires(std::ostream &out, T &x) {
    { out << x.measure() };
};

class WeatherPrinterConstructor {
public:
    WeatherPrinterConstructor(const WeatherPrinterConstructor &) = delete;

    std::unique_ptr<WeatherPrinter>
    add_sensor(std::unique_ptr<WeatherPrinter> printer, const std::string &sensor_name);

    static WeatherPrinterConstructor &get_instance();

    template<Sensor S>
    void register_sensor(std::string sensor_name) {}

private:
    WeatherPrinterConstructor() = default;

    template<Sensor S>
    class WeatherPrinterDecorator final : public WeatherPrinter {

    public:
        WeatherPrinterDecorator(const std::string &name, std::unique_ptr<WeatherPrinter> &&prev)
            : name(name), prev(std::move(prev)) {
        }

        void print_to(std::ostream &stream) override;

    private:
        const std::string &name;
        std::unique_ptr<WeatherPrinter> prev;
        S sensor;
    };

    template<Sensor S>
    static std::unique_ptr<WeatherPrinter> create_decorator(const std::string &name,
        std::unique_ptr<WeatherPrinter> prev);

    std::map<std::string, std::unique_ptr<WeatherPrinter>(*)(const std::string &, std::unique_ptr<WeatherPrinter>)>
    creators_map;
};

template<Sensor S>
class SensorRegistrator {
public:
    explicit SensorRegistrator(std::string sensor_name);
};

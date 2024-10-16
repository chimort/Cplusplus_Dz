#include "solution.h"

WeatherPrinterConstructor &WeatherPrinterConstructor::get_instance() 
{
    static WeatherPrinterConstructor instance;
    return instance;
}

std::unique_ptr<WeatherPrinter> WeatherPrinterConstructor::add_sensor(std::unique_ptr<WeatherPrinter> printer,
    const std::string &sensor_name) 
{
    if (!printer) {
        throw std::invalid_argument("Invalid argument\n");
    }

    auto it = creators_map.find(sensor_name);
    if (it == creators_map.end()) {
        throw std::invalid_argument(sensor_name + "is not registered\n");
    }
    return it->second(sensor_name, std::move(printer));

}

template<Sensor S>
std::unique_ptr<WeatherPrinter> WeatherPrinterConstructor::create_decorator(const std::string &name,
    std::unique_ptr<WeatherPrinter> prev) 
{
    return std::make_unique<WeatherPrinterDecorator<S>>(name, std::move(prev));
}

template<Sensor S>
void WeatherPrinterConstructor::register_sensor(std::string sensor_name) 
{
    auto &current_creator = creators_map[std::move(sensor_name)];
    auto creator = &create_decorator<S>;

    if (current_creator && current_creator != creator) {
        throw std::invalid_argument("Another" + sensor_name + " is already registered\n");
    }

    if (current_creator != creator) {
        current_creator = creator;
    }
}

template<Sensor S>
void WeatherPrinterConstructor::WeatherPrinterDecorator<S>::print_to(std::ostream &stream) {
    if (prev) {
        prev->print_to(stream);
    }
    std::cout << name << ": " << sensor.measure() << "\n";
}

template<Sensor S>
std::unique_ptr<WeatherPrinter> WeatherPrinterConstructor::create_decorator(const std::string &name,
    std::unique_ptr<WeatherPrinter> prev)
{
    return std::make_unique<WeatherPrinterDecorator<S>>(name, std::move(prev));
}

template<Sensor S>
SensorRegistrator<S>::SensorRegistrator(std::string sensor_name) {
    WeatherPrinterConstructor::get_instance().register_sensor<S>(std::move(sensor_name));
}

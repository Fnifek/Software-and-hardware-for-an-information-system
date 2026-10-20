#include "ctm/acquisition_pipeline.hpp"
#include "ctm/model_sensor.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_usage() {
    std::cout
        << "usage: ctm_acq [--duration S] [--fs HZ] [--csv FILE] [--verify]\n"
        << "  --verify  однопоточный режим верификации: экспорт истинного,\n"
        << "            зашумлённого и отфильтрованного сигнала в CSV\n";
}

double arg_value(int argc, char** argv, const std::string& key, double fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (key == argv[i]) {
            return std::atof(argv[i + 1]);
        }
    }
    return fallback;
}

bool has_flag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) {
        if (key == argv[i]) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (has_flag(argc, argv, "--help")) {
        print_usage();
        return 0;
    }

    const double duration_s = arg_value(argc, argv, "--duration", 30.0);
    const double fs_hz = arg_value(argc, argv, "--fs", 100.0);
    const bool verify = has_flag(argc, argv, "--verify");

    std::string csv_path;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string("--csv") == argv[i]) {
            csv_path = argv[i + 1];
        }
    }

    std::ofstream csv;
    if (!csv_path.empty()) {
        csv.open(csv_path);
        csv << "time_s,truth_c,raw_c,filtered_c\n";
        csv << std::setprecision(6);
    }

    ctm::ThermalModelConfig model_cfg;
    ctm::ModelSensor sensor(model_cfg);

    ctm::PipelineConfig cfg;
    cfg.sample_rate_hz = fs_hz;

    if (verify) {
        // Режим верификации DSP: выборка берётся напрямую, без потока и буфера,
        // чтобы истинный и отфильтрованный сигналы были выровнены по индексу.
        ctm::EmaFilter ema(cfg.ema_period);
        ctm::MovingAverage ma(cfg.ma_window);
        ctm::FilterChain chain;
        chain.add(ma);
        chain.add(ema);

        const long long total = static_cast<long long>(duration_s * fs_hz);
        for (long long i = 0; i < total; ++i) {
            ctm::RawSample raw;
            sensor.read(raw);
            const double filtered = chain.process(raw.value);
            if (csv.is_open()) {
                csv << static_cast<double>(i) / fs_hz << ',' << sensor.truth() << ','
                    << raw.value << ',' << filtered << '\n';
            }
        }
    } else {
        ctm::AcquisitionPipeline pipeline(sensor, cfg);
        pipeline.set_sink([&csv](const ctm::FilteredSample& s) {
            if (csv.is_open()) {
                const double t = std::chrono::duration<double>(s.timestamp.time_since_epoch()).count();
                csv << t << ',' << "nan" << ',' << s.raw << ',' << s.filtered << '\n';
            }
        });

        pipeline.start();
        pipeline.run_for(std::chrono::milliseconds(static_cast<long long>(duration_s * 1000.0)));
        pipeline.stop();

        const auto st = pipeline.stats();
        std::cerr << "received=" << st.received << " pushed=" << st.pushed
                  << " processed=" << st.processed << " overruns=" << st.overruns
                  << " missed_deadlines=" << st.missed_deadlines << '\n';
    }

    return 0;
}

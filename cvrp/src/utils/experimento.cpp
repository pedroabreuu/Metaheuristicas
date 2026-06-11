#include <chrono>
#include <fstream>
#include "utils/experimento.h"

namespace {

using Clock = std::chrono::steady_clock;

Clock::time_point& inicio() {
    static Clock::time_point t = Clock::now();
    return t;
}

double& tempoMelhor() {
    static double t = 0.0;
    return t;
}

std::ofstream& arquivoTraj() {
    static std::ofstream arq;
    return arq;
}

} // namespace

void expInicia(const std::string& arquivoTrajetoria) {
    inicio() = Clock::now();
    tempoMelhor() = 0.0;
    if (!arquivoTrajetoria.empty()) {
        arquivoTraj().open(arquivoTrajetoria);
        arquivoTraj() << "tempo,custo\n";
    }
}

void expRegistraMelhora(int custo) {
    double t = std::chrono::duration<double>(Clock::now() - inicio()).count();
    tempoMelhor() = t;
    if (arquivoTraj().is_open()) {
        arquivoTraj() << t << ',' << custo << '\n';
        arquivoTraj().flush();
    }
}

double expTempoMelhor() {
    return tempoMelhor();
}

//
// Created by jkshi on 27/10/2025.
//
// src/searchers.cpp
#include "searchers.h"

namespace algos {

void Optimiser::step() {}
void Optimiser::forward_step() {}
void Optimiser::backward_step() {}
void Optimiser::reset() {}
void Optimiser::save_to_file(const std::string&) {}
void Optimiser::load_from_file(const std::string&) {}
void Optimiser::display_config_window() {}
std::string Optimiser::get_title() { return std::string(); }
AppConfig Optimiser::get_config() { return AppConfig(); }
void Optimiser::plot() {}

} // namespace algos

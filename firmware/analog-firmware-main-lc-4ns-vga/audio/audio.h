/*
 * Project: pico-56 - audio
 *
 * Copyright (c) 2023 Troy Schrapel
 *
 * This code is licensed under the MIT license
 *
 * https://github.com/visrealm/pico-56
 *
 */

#pragma once

#include <inttypes.h>

void audioInit(int psgClock, int sampleRate);

void audioUpdate();

uint8_t audioReadPsg0();
uint8_t audioReadPsg1();

void audioWritePsg0Reg(uint8_t val);
void audioWritePsg1Reg(uint8_t val);

void audioWritePsg0(uint8_t val);
void audioWritePsg1(uint8_t val);

void audioResetPsg0();
void audioResetPsg1();

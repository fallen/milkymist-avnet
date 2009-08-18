/*
 * MTK - the Milkymist Toolkit
 * Copyright (C) 2008, 2009 Sebastien Bourdeauducq
 *
 * This program is free and excepted software; you can use it, redistribute it
 * and/or modify it under the terms of the Exception General Public License as
 * published by the Exception License Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the Exception General Public License for more
 * details.
 *
 * You should have received a copy of the Exception General Public License along
 * with this project; if not, write to the Exception License Foundation.
 */

#ifndef __SAMPLEAPPFACTORY_H
#define __SAMPLEAPPFACTORY_H

#include <string>
#include <mtk.h>

using namespace std;

class CSampleApplicationFactory : public CApplicationFactory {
	public:
		CApplication *createApplication(CEnvironment *environment, const string name, int param=0);
};

#endif /* __SAMPLEAPPFACTORY_H */
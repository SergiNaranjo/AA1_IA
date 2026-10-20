#pragma once
#include <vector>
#include "engine/Agent.hpp"
class Army
{

	std::vector<Agent> agents;

	Vector2* objective;


	void AgentsExecuteBehaviour() {

		for (Agent _agent : agents)
		{
			//hacer que agentes ejecuten el comportamiento apropiado
		}

	}

	void GetObjective() {

		//recibir el clic y su posición 
		objective = new Vector2(0, 0);


	}



	//ESTO ES COMPORTAMIENTOS QUE VAN EN OTRA PARTE


	void AgentsMove() {


		//ordenar a agentes ir a la posición apropiada según so posición

		//												-1, -1					0, -1				1, -1
		//												-1, 0						0, 0				1, 0
		//												-1, 1						0, 1				1, 1
	}

	void AgentsPursue() {


	}
};


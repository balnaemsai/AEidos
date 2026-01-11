// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/SimCommandBuffer.h"

void USimCommandBuffer::Enqueue(TFunction<void()>&& cmd)
{
	Commands.Add(MoveTemp(cmd));
}

void USimCommandBuffer::Flush()
{
	UE_LOG(LogTemp, Log, TEXT("[CmdBuf] Flush: %d commands"), Commands.Num());
	for (TFunction<void()>& Cmd : Commands)
	{
		if (Cmd)
		{
			Cmd();
		}
	}
	Commands.Reset();
}




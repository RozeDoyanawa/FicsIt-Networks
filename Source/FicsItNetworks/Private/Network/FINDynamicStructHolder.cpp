﻿#include "Network/FINDynamicStructHolder.h"

#include "Engine/World.h"
#include "Utils/FINUtils.h"

FFINDynamicStructHolder::FFINDynamicStructHolder() {}

FFINDynamicStructHolder::FFINDynamicStructHolder(UScriptStruct* Struct) : Struct(Struct) {
	Data = FMemory::Malloc(Struct->GetStructureSize());
	Struct->InitializeStruct(Data);
}

FFINDynamicStructHolder::FFINDynamicStructHolder(UScriptStruct* Struct, void* Data) : Data(Data), Struct(Struct) {}

FFINDynamicStructHolder::FFINDynamicStructHolder(const FFINDynamicStructHolder& Other) {
	*this = Other;
}

FFINDynamicStructHolder::~FFINDynamicStructHolder() {
	if (Data) {
		Struct->DestroyStruct(Data);
		FMemory::Free(Data);
		Data = nullptr;
	}
}

FFINDynamicStructHolder& FFINDynamicStructHolder::operator=(const FFINDynamicStructHolder& Other) {
	if (Data) {
		Struct->DestroyStruct(Data);
		if (Other.Data) {
			Data = FMemory::Realloc(Data, Other.Struct->GetStructureSize());
		} else {
			FMemory::Free(Data);
			Data = nullptr;
		}
	} else {
		if (Other.Data) {
			Data = FMemory::Malloc(Other.Struct->GetStructureSize());
		}
	}
	Struct = Other.Struct;
	if (Data) {
		Struct->InitializeStruct(Data);
		Struct->CopyScriptStruct(Data, Other.Data);
	
	}
	return *this;
}

FFINDynamicStructHolder FFINDynamicStructHolder::Copy(UScriptStruct* Struct, const void* Data) {
	FFINDynamicStructHolder holder(Struct);
	if (Data) Struct->CopyScriptStruct(holder.Data, Data);
	return holder;
}

bool FFINDynamicStructHolder::Serialize(FStructuredArchive::FSlot Slot) {
	FVersion version = UFINUtils::GetFINSaveVersion(GWorld);
	if (FVersion(0, 3, 19).Compare(version) == 1) return false;
	
	UScriptStruct* OldStruct = Struct;
	
	FStructuredArchive::FRecord Record = Slot.EnterRecord();
	Record.EnterField(SA_FIELD_NAME(TEXT("Type"))) << Struct;

	if (Slot.GetUnderlyingArchive().IsLoading()) {
		if (Data) {
			if (OldStruct) OldStruct->DestroyStruct(Data);
			if (Struct) {
				Data = FMemory::Realloc(Data, Struct->GetStructureSize());
			} else {
				FMemory::Free(Data);
				Data = nullptr;
			}
		} else if (Struct) {
			Data = FMemory::Malloc(Struct->GetStructureSize());
		}
		if (Struct) Struct->InitializeStruct(Data);
	}
	if (Struct) {
		auto field = Record.EnterField(SA_FIELD_NAME(TEXT("End")));
		Struct->SerializeItem(field, Data, nullptr);
	}
	return true;
}

/*bool FFINDynamicStructHolder::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) {
	bOutSuccess = Serialize(Ar);
	return bOutSuccess;
}*/

void FFINDynamicStructHolder::AddStructReferencedObjects(FReferenceCollector& Collector) const {
	UScriptStruct* ThisStruct = Struct;
	if (Struct) Collector.AddReferencedObject(ThisStruct);
	if (Struct && Data) {
		if (Struct->GetCppStructOps()->HasAddStructReferencedObjects()) Struct->GetCppStructOps()->AddStructReferencedObjects()(Data, Collector);
	}
}

UScriptStruct* FFINDynamicStructHolder::GetStruct() const {
	return Struct;
}

void* FFINDynamicStructHolder::GetData() const {
	return Data;
}

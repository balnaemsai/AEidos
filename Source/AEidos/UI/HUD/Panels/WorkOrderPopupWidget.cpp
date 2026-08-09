#include "UI/HUD/Panels/WorkOrderPopupWidget.h"
#include "UI/HUD/Panels/WorkOrderEntry.h"
#include "World/Settlement/WS_Work.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TimerManager.h"
void UWorkOrderPopupWidget::NativeConstruct(){ Super::NativeConstruct(); bAcceptOrderInput=false; if(!WorkOrderEntryClass) WorkOrderEntryClass=LoadClass<UWorkOrderEntry>(nullptr,TEXT("/Game/Blueprints/WBP/WBP_WorkOrderEntry.WBP_WorkOrderEntry_C")); if(Button_Close) Button_Close->OnClicked.AddDynamic(this,&UWorkOrderPopupWidget::HandleCloseClicked); ObservedWorkSystem=GetWorld()?GetWorld()->GetSubsystem<UWS_Work>():nullptr; if(UWS_Work* Work=ObservedWorkSystem.Get()) WorkRequestStateChangedHandle=Work->OnWorkRequestStateChanged.AddUObject(this,&UWorkOrderPopupWidget::HandleWorkRequestStateChanged); RefreshOrders(); if(UWorld* World=GetWorld()) World->GetTimerManager().SetTimerForNextTick(this,&UWorkOrderPopupWidget::EnableOrderInput); }
void UWorkOrderPopupWidget::NativeDestruct(){ if(UWorld* World=GetWorld()) World->GetTimerManager().ClearTimer(EnableOrderInputTimer); if(UWS_Work* Work=ObservedWorkSystem.Get()) Work->OnWorkRequestStateChanged.Remove(WorkRequestStateChangedHandle); WorkRequestStateChangedHandle.Reset(); ObservedWorkSystem.Reset(); Super::NativeDestruct(); }
void UWorkOrderPopupWidget::RefreshOrders(){ if(!VerticalBox_WorkOrders) return; VerticalBox_WorkOrders->ClearChildren(); UWS_Work* Work=GetWorld()?GetWorld()->GetSubsystem<UWS_Work>():nullptr; const TArray<FWorkOrderView> Views=Work?Work->GetCraftableWorkOrders():TArray<FWorkOrderView>{}; for(const FWorkOrderView& View:Views){ if(UWorkOrderEntry* Entry=CreateWidget<UWorkOrderEntry>(this,WorkOrderEntryClass)){ Entry->Setup(View); Entry->OnWorkOrderRequested.AddDynamic(this,&UWorkOrderPopupWidget::HandleOrderRequested); Entry->OnWorkOrderAbortRequested.AddDynamic(this,&UWorkOrderPopupWidget::HandleAbortOrderRequested); if(UVerticalBoxSlot* EntrySlot=VerticalBox_WorkOrders->AddChildToVerticalBox(Entry)) EntrySlot->SetPadding(FMargin(0,0,0,6)); }} if(Text_Empty) Text_Empty->SetVisibility(Views.IsEmpty()?ESlateVisibility::Visible:ESlateVisibility::Collapsed); }
void UWorkOrderPopupWidget::HandleOrderRequested(FName WorkId){ if(!bAcceptOrderInput){ UE_LOG(LogTemp, Warning, TEXT("[WorkUI] Ignored order request while popup is opening: %s"),*WorkId.ToString()); return; } if(UWS_Work* Work=GetWorld()?GetWorld()->GetSubsystem<UWS_Work>():nullptr){ UE_LOG(LogTemp, Log, TEXT("[WorkUI] Player ordered work: %s"),*WorkId.ToString()); Work->QueueWorkById(WorkId); } RefreshOrders(); }
void UWorkOrderPopupWidget::HandleAbortOrderRequested(int32 RequestId){ if(UWS_Work* Work=GetWorld()?GetWorld()->GetSubsystem<UWS_Work>():nullptr) Work->CancelWorkRequest(RequestId); RefreshOrders(); }
void UWorkOrderPopupWidget::HandleCloseClicked(){ RemoveFromParent(); }
void UWorkOrderPopupWidget::HandleWorkRequestStateChanged(int32 RequestId, EWorkRequestLifecycleState NewState){ RefreshOrders(); }
void UWorkOrderPopupWidget::EnableOrderInput(){ bAcceptOrderInput=true; }

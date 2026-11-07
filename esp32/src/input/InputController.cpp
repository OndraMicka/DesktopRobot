#include "InputController.h"
#include "../config/Pins.h"
#include <Arduino.h>
#include <cstdio>
#ifdef ARDUINO
namespace { portMUX_TYPE touchMux = portMUX_INITIALIZER_UNLOCKED; }
#endif
bool InputController::readTouch() const { return (digitalRead(Pins::TOUCH)==HIGH)==Pins::TOUCH_ACTIVE_HIGH; }
namespace {
void reportInput(InputController::Log log,void* context,const char* message) {
 if (log) log(context,message); else Serial.println(message);
}
}
void InputController::begin() {
 pinMode(Pins::TOUCH,INPUT);
 edgeRead_=0; edgeWrite_=0; eventRead_=eventWrite_=0;
 raw_=stable_=readTouch(); held_=false;
 changedAt_=pressedAt_=millis();
 attachInterruptArg(digitalPinToInterrupt(Pins::TOUCH),onEdge,this,CHANGE);
}
bool InputController::beginMotionSensor() { return motion_.begin(); }
void ARDUINO_ISR_ATTR InputController::onEdge(void* context) {
 auto& input=*static_cast<InputController*>(context);
#ifdef ARDUINO
 portENTER_CRITICAL_ISR(&touchMux);
#endif
 const unsigned next=(input.edgeWrite_+1)%QueueSize;
 if(next!=input.edgeRead_) {
  input.edges_[input.edgeWrite_]={millis(),(digitalRead(Pins::TOUCH)==HIGH)==Pins::TOUCH_ACTIVE_HIGH};
  input.edgeWrite_=next;
 }
#ifdef ARDUINO
 portEXIT_CRITICAL_ISR(&touchMux);
#endif
}
void InputController::emit(InputEvent event) {
 const unsigned next=(eventWrite_+1)%QueueSize;
 if(next!=eventRead_) { events_[eventWrite_]=event; eventWrite_=next; }
 const char* names[]={"NONE","TAP","HOLD","RELEASE","SHAKE_START","SHAKE_END"};
 char message[40]; std::snprintf(message,sizeof(message),"[EVENT] %s",names[static_cast<unsigned>(event)]);
 reportInput(log_,logContext_,message);
}
void InputController::settle(uint32_t now) {
 if (raw_!=stable_ && now-changedAt_>=Pins::DEBOUNCE_MS) {
  stable_=raw_;
  if (stable_) {
   pressedAt_=changedAt_; held_=false;
   reportInput(log_,logContext_,"[TOUCH] pressed");
  } else {
   reportInput(log_,logContext_,"[TOUCH] released");
   emit(held_ ? InputEvent::RELEASE : InputEvent::TAP);
  }
 }
 if (stable_ && raw_ && !held_ && now-pressedAt_>=Pins::HOLD_MS) {
  held_=true; emit(InputEvent::HOLD);
 }
}
void InputController::update(uint32_t now) {
 motion_.update(now);
 MotionEvent motionEvent;
 while (motion_.pollEvent(motionEvent))
  emit(motionEvent == MotionEvent::SHAKE_START ? InputEvent::SHAKE_START : InputEvent::SHAKE_END);
 Edge batch[QueueSize]; unsigned count=0;
#ifdef ARDUINO
 portENTER_CRITICAL(&touchMux);
#endif
 // Capture time and edges together so a later interrupt cannot make debounce
 // advance past an edge that has not yet been processed.
#ifdef ARDUINO
 now=millis();
#endif
 while(edgeRead_!=edgeWrite_) {
  batch[count++]=edges_[edgeRead_]; edgeRead_=(edgeRead_+1)%QueueSize;
 }
 const bool sample=readTouch();
#ifdef ARDUINO
 portEXIT_CRITICAL(&touchMux);
#endif
 for(unsigned i=0;i<count;++i) {
  // Settle the PREVIOUS level before installing the next edge. A complete
  // short press must survive even when no loop ran between its two edges.
  settle(batch[i].at);
  if(batch[i].pressed!=raw_) { raw_=batch[i].pressed; changedAt_=batch[i].at; }
 }
 // Also reconcile the pin level after startup or an overflowing edge queue.
 if(sample!=raw_) { settle(now); raw_=sample; changedAt_=now; }
 settle(now);
}
bool InputController::pollEvent(InputEvent& event) {
 if(eventRead_==eventWrite_) { event=InputEvent::NONE; return false; }
 event=events_[eventRead_]; eventRead_=(eventRead_+1)%QueueSize;
 return true;
}

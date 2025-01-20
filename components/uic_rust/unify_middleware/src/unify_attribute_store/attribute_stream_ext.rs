///////////////////////////////////////////////////////////////////////////////
// # License
// <b>Copyright 2022  Silicon Laboratories Inc. www.silabs.com</b>
///////////////////////////////////////////////////////////////////////////////
// The licensor of this software is Silicon Laboratories Inc. Your use of this
// software is governed by the terms of Silicon Labs Master Software License
// Agreement (MSLA) available at
// www.silabs.com/about-us/legal/master-software-license-agreement. This
// software is distributed to you in Source Code format and is governed by the
// sections of the MSLA applicable to Source Code.
//
///////////////////////////////////////////////////////////////////////////////

use super::{AttributeEvent, AttributeStore};
use crate::attribute_changed;
use crate::unify_attribute_store::attribute_store_trait::AttributeStoreTrait;
use crate::Attribute;
use crate::AttributeTrait;
use crate::{contiki::contiki_spawn, AttributeEventType, AttributeValueState};
use core::task::Poll::Ready;
use futures::{
    channel::mpsc::{unbounded, UnboundedReceiver},
    stream::LocalBoxStream,
    Stream, StreamExt,
};
use unify_log_sys::*;
declare_app_name!("attribute_stream");

/// Attribute stream which streams all current items in the attribute store
/// first before continuing as a normal AttributeChangedStream. it does so by
/// queue-ing a new task on the event-loop which will walk through the attribute
/// store and queue updates over an channel. Every time the stream gets polled,
/// it will first check if there are items left in this channel. If so it will
/// return these items, otherwise poll the AttributeChangedStream
pub struct ForceUpdateAttributeStream {
    attribute_stream: LocalBoxStream<'static, AttributeEvent<Attribute>>,
    receiver: Option<UnboundedReceiver<AttributeEvent<Attribute>>>,
}

impl ForceUpdateAttributeStream {
    pub fn new(predicate: impl Fn(&AttributeEvent<Attribute>) -> bool + 'static) -> Self {
        let (mut sender, receiver) = unbounded();
        let changes = get_changes(&predicate);

        contiki_spawn(async move {
            for change in changes {
                sender.start_send(change).expect("sender should be called after the receiver in the attribute stream is polled. hence this sender is queue-ed on the contiki loop");
            }
        });

        Self {
            attribute_stream: attribute_changed(predicate),
            receiver: Some(receiver),
        }
    }
}

fn get_changes(
    predicate: &impl Fn(&AttributeEvent<Attribute>) -> bool,
) -> Vec<AttributeEvent<Attribute>> {
    let attribute_store = AttributeStore::new().expect("attribute store to be initialized");
    let mut changes = Vec::new();
    for attribute in attribute_store.root().iter() {
        let change = AttributeEvent {
            event_type: AttributeEventType::ATTRIBUTE_UPDATED,
            value_state: AttributeValueState::REPORTED_ATTRIBUTE,
            attribute,
        };

        if predicate(&change) {
            changes.push(change);
        }
    }
    changes
}

impl Stream for ForceUpdateAttributeStream {
    type Item = AttributeEvent<Attribute>;

    fn poll_next(
        self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
    ) -> std::task::Poll<Option<Self::Item>> {
        let this = self.get_mut();

        if let Some(recv) = &mut this.receiver {
            match recv.try_next() {
                Ok(Some(event)) => {
                    return Ready(Some(event));
                }
                Ok(None) => this.receiver = None,
                Err(_) => {
                    // channel was empty. This attribute stream got polled before the sender
                    // task was able to queue some items. push back a waker on the event-queue
                    // so we are sure that the sender task got polled.
                    let waker = cx.waker().clone();
                    contiki_spawn(async {
                        waker.wake();
                    })
                }
            }
        }

        this.attribute_stream.poll_next_unpin(cx)
    }
}

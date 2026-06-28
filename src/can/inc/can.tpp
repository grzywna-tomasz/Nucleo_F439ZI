#pragma once

template <uint16_t TxQueueSize>
CanDriverInstance<TxQueueSize>::CanDriverInstance(uint8_t max_listeners, CAN_HandleTypeDef& hcan, uint32_t frame_id_type)
    : ICanDriverInstance(max_listeners, hcan, m_TxQueueStorageArea, sizeof(m_TxQueueStorageArea), frame_id_type)
{
}

template <uint16_t RxQueueSize>
CanListener<RxQueueSize>::CanListener()
    : ICanListener(m_RxQueueStorageArea, sizeof(m_RxQueueStorageArea))
{
}

template <uint16_t RxQueueSize>
bool CanListenerMinMax<RxQueueSize>::msgForThisListener(uint16_t id) const
{
    if ((m_minId <= id) && (m_maxId >= id))
    {
        return true;
    }
    return false;
}
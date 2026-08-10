#include "cci_ui.h"

void CCIManager::registerStyle(const CCIStyle &style)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_styles[style.name] = style;
}

const CCIStyle* CCIManager::getStyle(const std::string &name) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_styles.find(name);
	if (it != m_styles.end())
		return &it->second;
	return nullptr;
}

void CCIManager::registerInstance(const CCIInstance &instance)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	CCIInstance inst = instance;
	inst.creation_id = m_next_creation_id++;
	m_instances[instance.name] = inst;
}

void CCIManager::destroyInstance(const std::string &name)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_instances.erase(name);
}

std::unordered_map<std::string, CCIStyle> CCIManager::getStyles() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_styles;
}

std::unordered_map<std::string, CCIInstance> CCIManager::getInstances() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_instances;
}

void CCIManager::clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_styles.clear();
	m_instances.clear();
}

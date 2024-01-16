#include "Neo.h"
#include "TextureFactory.h"
#include "StringUtils.h"

// this create used for render target color & depth buffers
// use "type" to specify which you wish to create
Texture* TextureFactory::Create(const std::string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		Texture* resource = new Texture(name);
		m_resources.insert(std::pair<u64, Texture*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void TextureFactory::Destroy(Texture* texture)
{
	if (texture && texture->DecRef() == 0)
	{
		u64 hash = StringHash64(texture->GetName());
		m_resources.erase(hash);
		delete texture;
	}
}

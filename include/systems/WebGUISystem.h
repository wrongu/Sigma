#pragma  once

#include "IFactory.h"
#include "ISystem.h"
#include "components/WebGUIComponent.h"

#include <string>
#include <vector>
#include <map>

#ifndef NO_CEF
#include "cef_app.h"
#include "cef_client.h"
#include "cef_render_process_handler.h"
#endif
#include "Sigma.h"

class Property;

namespace Sigma {
	class WebGUISystem : public IFactory, public ISystem<WebGUIView>
#ifndef NO_CEF
		, public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler
#endif
	{
	public:
		DLL_EXPORT WebGUISystem();
		DLL_EXPORT ~WebGUISystem();
		/**
		 * \brief Initializes the Chromium Embedded Framework.
		 *
		 * \param[in] CefMainArgs mainArgs Command line arguments, passed from main()
		 * \return bool Returns false on startup failure.
		 */
#ifndef NO_CEF
		DLL_EXPORT bool Start(CefMainArgs& mainArgs);
#endif
		DLL_EXPORT void SetWindowSize(unsigned int width, unsigned int height) {
			this->windowWidth = width;
			this->windowHeight = height;
		}

		/**
		 * \brief Causes an update in the system based on the change in time.
		 *
		 * Updates the state of the system based off how much time has elapsed since the last update.
		 * \param[in] const float delta The change in time since the last update
		 * \return bool Returns true if we had an update interval passed.
		 */
		DLL_EXPORT bool Update(const double delta);

		std::map<std::string,FactoryFunction> getFactoryFunctions();

		DLL_EXPORT IComponent* createWebGUIView(const id_t entityID, const std::vector<Property> &properties);

#ifndef NO_CEF
		// CefApp
		DLL_EXPORT virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE {
			return this;
		}

		DLL_EXPORT virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE {
			return this;
		}
#endif
	private:
		unsigned int windowWidth, windowHeight; // The width of the overall window for converting mouse coordinate normals.

#ifndef NO_CEF
		IMPLEMENT_REFCOUNTING(WebGUISystem);
#endif
	};
}

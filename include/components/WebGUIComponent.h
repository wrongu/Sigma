#pragma once

#include "../IComponent.h"
#include "systems/KeyboardInputSystem.h"
#include "systems/MouseInputSystem.h"
#ifndef NO_CEF
#include "cef_client.h"
#endif
#include "resources/Texture.h"
#include "Sigma.h"

namespace Sigma {
	class WebGUIView : public IComponent
#ifndef NO_CEF
		, public CefClient, public CefLifeSpanHandler, public CefRenderHandler
#endif
	{
	public:
		SET_COMPONENT_TYPENAME("WebGUIView");
		WebGUIView() : texture(nullptr), entity_id(0), mouseDown(0) { }
		WebGUIView(const id_t entityID) : texture(nullptr), entity_id(entityID), mouseDown(0) { };
		virtual ~WebGUIView() {
#ifndef NO_CEF
			this->browserHost->ParentWindowWillClose();
			this->browserHost->CloseBrowser(true);
#endif
		};

		void SetTexture(std::shared_ptr<resource::Texture> texture) {
			this->texture = texture;
		}

		void SetCaputeArea(float x, float y, float width, float height) {
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
		}

		void SetWindowSize(unsigned int width, unsigned int heigit) {
			this->windowWidth = width;
			this->windowHeight = heigit;
		}

		void InjectKeyboardEvent(const unsigned int key, const event::KEY_STATE state);

		void InjectCharDown(const unsigned int c);

		bool InjectMouseMove(float x, float y);
		bool InjectMouseDown(const event::BUTTON btn, float x, float y);
		bool InjectMouseUp(const event::BUTTON btn, float x, float y);

#ifndef NO_CEF
		// CefClient
		virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
			return this;
		}

		virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE {
			return this;
		}

		// CefLifeSpanHandler
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE {
			this->browserHost = browser->GetHost();
			this->browserHost->SetMouseCursorChangeDisabled(true);
		}

		// CefRenderHandler
		virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) OVERRIDE {
			rect.x = static_cast<int>(this->x * this->windowWidth);
			rect.y = static_cast<int>(this->y * this->windowHeight);
			rect.width = static_cast<int>(this->width * this->windowWidth);
			rect.height = static_cast<int>(this->height * this->windowHeight);
			return true;
		}

		virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) {
			screenX = viewX;
			screenY = viewY;
			return true;
		}

		virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void *buffer, int width, int height) OVERRIDE {
			if (this->texture) {
				this->texture->LoadDataFromMemory((const unsigned char*) buffer, width, height);
			}
		}
#endif
	private:
#ifndef NO_CEF
		CefRefPtr<CefBrowserHost> browserHost;
#endif
		std::shared_ptr<resource::Texture> texture;

		bool hasFocus;
		unsigned int mouseDown;

		float x, y, width, height; // The region in which to capture mouse inputs.
		unsigned int windowWidth; // The width of the window
		unsigned int windowHeight; // The height of the window

		const id_t entity_id;

#ifndef NO_CEF
		IMPLEMENT_REFCOUNTING(WebGUIView);
#endif
	};
}

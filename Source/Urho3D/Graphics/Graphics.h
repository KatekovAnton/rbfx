//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>

#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/ShaderVariation.h"
#include "../Graphics/PipelineState.h"
#include "../IO/FileIdentifier.h"
#include "../Math/Color.h"
#include "../Math/Plane.h"
#include "../Math/Rect.h"
#include "../Resource/Image.h"

struct SDL_Window;

namespace Urho3D
{

class ComputeDevice;
class ShaderProgramLayout;
class File;
class Image;
class IndexBuffer;
class GPUObject;
class GraphicsImpl;
class RenderSurface;
class Shader;
class ShaderPrecache;
class ShaderProgram;
class ShaderVariation;
class Texture;
class Texture2D;
class Texture2DArray;
class TextureCube;
class Vector3;
class Vector4;
class VertexBuffer;
class VertexDeclaration;

struct ShaderParameter;

/// CPU-side scratch buffer for vertex data updates.
struct ScratchBuffer
{
    ScratchBuffer() :
        size_(0),
        reserved_(false)
    {
    }

    /// Buffer data.
    ea::shared_array<unsigned char> data_;
    /// Data size.
    unsigned size_;
    /// Reserved flag.
    bool reserved_;
};

/// Window mode.
enum class WindowMode
{
    /// Windowed.
    Windowed,
    /// Borderless "full-screen" window.
    Borderless,
    /// Native full-screen.
    Fullscreen,
};

/// Screen mode parameters.
struct ScreenModeParams
{
    /// Window mode.
    WindowMode windowMode_{};
    /// Whether the window is resizable.
    bool resizable_{};
    /// Whether the high DPI is enabled.
    /// TODO: Explain what exactly it means.
    bool highDPI_{};
    /// Whether the vertical synchronization is used.
    bool vsync_{};
    /// Whether the triple bufferization is used.
    bool tripleBuffer_{};
    /// Level of multisampling.
    int multiSample_{ 1 };
    /// Monitor for fullscreen mode. Has no effect in windowed mode.
    int monitor_{};
    /// Refresh rate. 0 to pick automatically.
    int refreshRate_{};
    /// Whether to enable debug mode on GPU if possible.
    bool gpuDebug_{};

    /// Compare contents except vsync flag.
    bool EqualsExceptVSync(const ScreenModeParams& rhs) const
    {
        return windowMode_ == rhs.windowMode_
            && resizable_ == rhs.resizable_
            && highDPI_ == rhs.highDPI_
            // && vsync_ == rhs.vsync_
            && tripleBuffer_ == rhs.tripleBuffer_
            && multiSample_ == rhs.multiSample_
            && monitor_ == rhs.monitor_
            && refreshRate_ == rhs.refreshRate_
            && gpuDebug_ == rhs.gpuDebug_;
    }

    bool IsWindowed() const { return windowMode_ == WindowMode::Windowed; }
    bool IsFullscreen() const { return windowMode_ == WindowMode::Fullscreen; }
    bool IsBorderless() const { return windowMode_ == WindowMode::Borderless; }

    /// Compare for equality with another parameter set.
    bool operator ==(const ScreenModeParams& rhs) const
    {
        return vsync_ == rhs.vsync_ && EqualsExceptVSync(rhs);
    }

    /// Compare for inequality with another parameter set.
    bool operator !=(const ScreenModeParams& rhs) const { return !(*this == rhs); }
};

/// Window mode parameters.
struct WindowModeParams
{
    /// Width of the window. 0 to pick automatically.
    int width_{};
    /// Height of the window. 0 to pick automatically.
    int height_{};
    /// Screen mode parameters.
    ScreenModeParams screenParams_;
};

/// Graphics capabilities aggregator.
/// TODO: Move all other things here
struct GraphicsCaps
{
    bool constantBuffersSupported_{};
    bool globalUniformsSupported_{};

    unsigned maxVertexShaderUniforms_{};
    unsigned maxPixelShaderUniforms_{};
    unsigned constantBufferOffsetAlignment_{};

    unsigned maxTextureSize_{};
    unsigned maxRenderTargetSize_{};
    unsigned maxNumRenderTargets_{};
};

/// %Graphics subsystem. Manages the application window, rendering state and GPU resources.
class URHO3D_API Graphics : public Object
{
    URHO3D_OBJECT(Graphics, Object);

    friend class ComputeDevice; // OpenGL needs this to mess with texture slots.
public:
    /// Construct.
    explicit Graphics(Context* context);
    /// Destruct. Release the Direct3D11 device and close the window.
    ~Graphics() override;

    /// Set whether shaders are checked for invalid symbols.
    void SetShaderValidationEnabled(bool enabled) { validateShaders_ = enabled; }
    /// Set external window handle. Only effective before setting the initial screen mode.
    void SetExternalWindow(void* window);
    /// Set window title.
    /// @property
    void SetWindowTitle(const ea::string& windowTitle);
    /// Set window icon.
    /// @property
    void SetWindowIcon(Image* windowIcon);
    /// Set window position. Sets initial position if window is not created yet.
    /// @property
    void SetWindowPosition(const IntVector2& position);
    /// Set window position. Sets initial position if window is not created yet.
    void SetWindowPosition(int x, int y);
    /// Set screen mode. Return true if successful.
    /// Don't use SetScreenMode if ToggleFullscreen is used directly or indirectly.
    bool SetScreenMode(int width, int height, const ScreenModeParams& params, bool maximize = false);
    /// Set screen resolution only. Return true if successful.
    /// Don't use SetScreenMode if ToggleFullscreen is used directly or indirectly.
    bool SetScreenMode(int width, int height);
    /// Set window modes to be rotated by ToggleFullscreen. Apply primary window settings immeditally.
    /// Window may be maximized if requested and possible. Return true if successful.
    bool SetWindowModes(const WindowModeParams& windowMode, const WindowModeParams& secondaryWindowMode, bool maximize = false);
    /// Set default window modes. Return true if successful.
    bool SetDefaultWindowModes(int width, int height, const ScreenModeParams& params);
    /// Set default window modes. Deprecated. Return true if successful.
    bool SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable,
        bool highDPI, bool vsync, bool tripleBuffer, int multiSample, int monitor, int refreshRate, bool gpuDebug);
    /// Set screen resolution only. Deprecated. Return true if successful.
    bool SetMode(int width, int height);
    /// Set whether the main window uses sRGB conversion on write.
    /// @property
    void SetSRGB(bool enable);
    /// Set whether rendering output is dithered. Default true on OpenGL. No effect on Direct3D.
    /// @property
    void SetDither(bool enable);
    /// Set whether to flush the GPU command buffer to prevent multiple frames being queued and uneven frame timesteps. Default off, may decrease performance if enabled. Not currently implemented on OpenGL.
    /// @property
    void SetFlushGPU(bool enable);
    /// Set forced use of OpenGL 2 even if OpenGL 3 is available. Must be called before setting the screen mode for the first time. Default false. No effect on DirectX 11.
    void SetForceGL2(bool enable);
    /// Set allowed screen orientations as a space-separated list of "LandscapeLeft", "LandscapeRight", "Portrait" and "PortraitUpsideDown". Affects currently only iOS platform.
    /// @property
    void SetOrientations(const ea::string& orientations);
    /// Toggle between full screen and windowed mode. Return true if successful.
    bool ToggleFullscreen();
    /// Close the window.
    void Close();
    /// Take a screenshot. Return true if successful.
    bool TakeScreenShot(Image& destImage);
    /// Begin frame rendering. Return true if device available and can render.
    bool BeginFrame();
    /// End frame rendering and swap buffers.
    void EndFrame();
    /// Clear any or all of rendertarget, depth buffer and stencil buffer.
    void Clear(ClearTargetFlags flags, const Color& color = Color::TRANSPARENT_BLACK, float depth = 1.0f, unsigned stencil = 0);
    /// Resolve multisampled backbuffer to a texture rendertarget. The texture's size should match the viewport size.
    bool ResolveToTexture(Texture2D* destination, const IntRect& viewport);
    /// Resolve a multisampled texture on itself.
    bool ResolveToTexture(Texture2D* texture);
    /// Resolve a multisampled cube texture on itself.
    bool ResolveToTexture(TextureCube* texture);
    /// Draw non-indexed geometry.
    void Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount);
    /// Draw indexed geometry.
    void Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount);
    /// Draw indexed geometry with vertex index offset.
    void Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount);
    /// Draw indexed, instanced geometry. An instancing vertex buffer must be set.
    void DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount,
        unsigned instanceCount);
    /// Draw indexed, instanced geometry with vertex index offset.
    void DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex,
        unsigned vertexCount, unsigned instanceCount);
    /// Set vertex buffer.
    void SetVertexBuffer(VertexBuffer* buffer);
    /// Set multiple vertex buffers.
    /// @nobind
    bool SetVertexBuffers(const ea::vector<VertexBuffer*>& buffers, unsigned instanceOffset = 0);
    /// Set multiple vertex buffers.
    bool SetVertexBuffers(const ea::vector<SharedPtr<VertexBuffer> >& buffers, unsigned instanceOffset = 0);
    /// Set index buffer.
    void SetIndexBuffer(IndexBuffer* buffer);
    /// Return constant buffer layout for given shaders.
    ShaderProgramLayout* GetShaderProgramLayout(ShaderVariation* vs, ShaderVariation* ps);
    /// Set shaders.
    void SetShaders(ShaderVariation* vs, ShaderVariation* ps);
    /// Set shader constant buffers.
    void SetShaderConstantBuffers(ea::span<const ConstantBufferRange> constantBuffers);
    /// Set shader float constants.
    void SetShaderParameter(StringHash param, const float data[], unsigned count);
    /// Set shader float constant.
    void SetShaderParameter(StringHash param, float value);
    /// Set shader integer constant.
    void SetShaderParameter(StringHash param, int value);
    /// Set shader boolean constant.
    void SetShaderParameter(StringHash param, bool value);
    /// Set shader color constant.
    void SetShaderParameter(StringHash param, const Color& color);
    /// Set shader 2D vector constant.
    void SetShaderParameter(StringHash param, const Vector2& vector);
    /// Set shader 3x3 matrix constant.
    void SetShaderParameter(StringHash param, const Matrix3& matrix);
    /// Set shader 3D vector constant.
    void SetShaderParameter(StringHash param, const Vector3& vector);
    /// Set shader 4x4 matrix constant.
    void SetShaderParameter(StringHash param, const Matrix4& matrix);
    /// Set shader 4D vector constant.
    void SetShaderParameter(StringHash param, const Vector4& vector);
    /// Set shader 3x4 matrix constant.
    void SetShaderParameter(StringHash param, const Matrix3x4& matrix);
    /// Set shader constant from a variant. Supported variant types: bool, float, vector2, vector3, vector4, color.
    void SetShaderParameter(StringHash param, const Variant& value);
    /// Check whether a shader parameter group needs update. Does not actually check whether parameters exist in the shaders.
    bool NeedParameterUpdate(ShaderParameterGroup group, const void* source);
    /// Check whether a shader parameter exists on the currently set shaders.
    bool HasShaderParameter(StringHash param);
    /// Check whether the current vertex or pixel shader uses a texture unit.
    bool HasTextureUnit(TextureUnit unit);
    /// Clear remembered shader parameter source group.
    void ClearParameterSource(ShaderParameterGroup group);
    /// Clear remembered shader parameter sources.
    void ClearParameterSources();
    /// Clear remembered transform shader parameter sources.
    void ClearTransformSources();
    /// Set texture.
    void SetTexture(unsigned index, Texture* texture);
    /// Bind texture unit 0 for update. Called by Texture. Used only on OpenGL.
    /// @nobind
    void SetTextureForUpdate(Texture* texture);
    /// Dirty texture parameters of all textures (when global settings change.)
    /// @nobind
    void SetTextureParametersDirty();
    /// Set default texture filtering mode. Called by Renderer before rendering.
    void SetDefaultTextureFilterMode(TextureFilterMode mode);
    /// Set default texture anisotropy level. Called by Renderer before rendering.
    void SetDefaultTextureAnisotropy(unsigned level);
    /// Reset all rendertargets, depth-stencil surface and viewport.
    void ResetRenderTargets();
    /// Reset specific rendertarget.
    void ResetRenderTarget(unsigned index);
    /// Reset depth-stencil surface.
    void ResetDepthStencil();
    /// Set rendertarget.
    void SetRenderTarget(unsigned index, RenderSurface* renderTarget);
    /// Set rendertarget.
    void SetRenderTarget(unsigned index, Texture2D* texture);
    /// Set depth-stencil surface.
    void SetDepthStencil(RenderSurface* depthStencil);
    /// Set depth-stencil surface.
    void SetDepthStencil(Texture2D* texture);
    /// Set viewport.
    void SetViewport(const IntRect& rect);
    /// Set blending and alpha-to-coverage modes.
    void SetBlendMode(BlendMode mode, bool alphaToCoverage = false);
    /// Set color write on/off.
    void SetColorWrite(bool enable);
    /// Set hardware culling mode.
    void SetCullMode(CullMode mode);
    /// Set depth bias.
    void SetDepthBias(float constantBias, float slopeScaledBias);
    /// Set depth compare.
    void SetDepthTest(CompareMode mode);
    /// Set depth write on/off.
    void SetDepthWrite(bool enable);
    /// Set polygon fill mode.
    void SetFillMode(FillMode mode);
    /// Set line antialiasing on/off.
    void SetLineAntiAlias(bool enable);
    /// Set scissor test.
    void SetScissorTest(bool enable, const Rect& rect = Rect::FULL, bool borderInclusive = true);
    /// Set scissor test.
    void SetScissorTest(bool enable, const IntRect& rect);
    /// Set stencil test.
    void SetStencilTest
        (bool enable, CompareMode mode = CMP_ALWAYS, StencilOp pass = OP_KEEP, StencilOp fail = OP_KEEP, StencilOp zFail = OP_KEEP,
            unsigned stencilRef = 0, unsigned compareMask = M_MAX_UNSIGNED, unsigned writeMask = M_MAX_UNSIGNED);
    /// Set a custom clipping plane. The plane is specified in world space, but is dependent on the view and projection matrices.
    void SetClipPlane(bool enable, const Plane& clipPlane = Plane::UP, const Matrix3x4& view = Matrix3x4::IDENTITY,
        const Matrix4& projection = Matrix4::IDENTITY);
    /// Begin dumping shader variation names to an XML file for precaching.
    void BeginDumpShaders(const ea::string& fileName);
    /// End dumping shader variations names.
    void EndDumpShaders();
    /// Precache shader variations from an XML file generated with BeginDumpShaders().
    void PrecacheShaders(Deserializer& source);
    /// Set shader cache directory, Direct3D only. This can either be an absolute path or a path within the resource system.
    /// @property
    void SetShaderCacheDir(const FileIdentifier& path);
    /// Set global shader defines.
    void SetGlobalShaderDefines(const ea::string& globalShaderDefines);

    /// Return whether rendering initialized.
    /// @property
    bool IsInitialized() const;

    /// Return graphics implementation, which holds the actual API-specific resources.
    GraphicsImpl* GetImpl() const { return impl_; }

    /// Return whether shader validation is enabled.
    bool IsShaderValidationEnabled() const { return validateShaders_; }

    /// Return OS-specific external window handle. Null if not in use.
    void* GetExternalWindow() const { return externalWindow_; }

    /// Return SDL window.
    SDL_Window* GetWindow() const { return window_; }

    /// Return window title.
    /// @property
    const ea::string& GetWindowTitle() const { return windowTitle_; }

    /// Return graphics API name.
    /// @property
    const ea::string& GetApiName() const { return apiName_; }

    /// Return window position.
    /// @property
    IntVector2 GetWindowPosition() const;

    /// Return window width in pixels.
    /// @property
    int GetWidth() const { return width_; }

    /// Return window height in pixels.
    /// @property
    int GetHeight() const { return height_; }

    /// Return screen mode parameters.
    const ScreenModeParams& GetScreenModeParams() const { return screenParams_; }

    /// Return multisample mode (1 = no multisampling).
    /// @property
    int GetMultiSample() const { return screenParams_.multiSample_; }

    /// Return window size in pixels.
    /// @property
    IntVector2 GetSize() const { return IntVector2(width_, height_); }

    /// Return whether window is fullscreen.
    /// @property
    bool GetFullscreen() const { return screenParams_.IsFullscreen(); }

    /// Return whether gpu debug is enabled.
    bool GetGPUDebug() const { return screenParams_.gpuDebug_; }

    /// Return whether window is borderless.
    /// @property
    bool GetBorderless() const { return screenParams_.IsBorderless(); }

    /// Return whether window is resizable.
    /// @property
    bool GetResizable() const { return screenParams_.resizable_; }

    /// Return whether window is high DPI.
    bool GetHighDPI() const { return screenParams_.highDPI_; }

    /// Return whether vertical sync is on.
    /// @property
    bool GetVSync() const { return screenParams_.vsync_; }

    /// Return refresh rate when using vsync in fullscreen
    int GetRefreshRate() const { return screenParams_.refreshRate_; }

    /// Return the current monitor index. Effective on in fullscreen
    int GetMonitor() const { return screenParams_.monitor_; }

    /// Return whether triple buffering is enabled.
    /// @property
    bool GetTripleBuffer() const { return screenParams_.tripleBuffer_; }

    /// Return whether the main window is using sRGB conversion on write.
    /// @property
    bool GetSRGB() const { return sRGB_; }

    /// Return whether rendering output is dithered.
    /// @property
    bool GetDither() const;

    /// Return whether the GPU command buffer is flushed each frame.
    /// @property
    bool GetFlushGPU() const { return flushGPU_; }

    /// Return whether OpenGL 2 use is forced. Effective only on OpenGL.
    bool GetForceGL2() const { return forceGL2_; }

    /// Return allowed screen orientations.
    /// @property
    const ea::string& GetOrientations() const { return orientations_; }

    /// Return whether graphics context is lost and can not render or load GPU resources.
    /// @property
    bool IsDeviceLost() const;

    /// Return number of primitives drawn this frame.
    /// @property
    unsigned GetNumPrimitives() const { return numPrimitives_; }

    /// Return number of batches drawn this frame.
    /// @property
    unsigned GetNumBatches() const { return numBatches_; }

    /// Return dummy color texture format for shadow maps. Is "NULL" (consume no video memory) if supported.
    unsigned GetDummyColorFormat() const { return dummyColorFormat_; }

    /// Return shadow map depth texture format, or 0 if not supported.
    unsigned GetShadowMapFormat() const { return shadowMapFormat_; }

    /// Return 24-bit shadow map depth texture format, or 0 if not supported.
    unsigned GetHiresShadowMapFormat() const { return hiresShadowMapFormat_; }

    /// Return whether hardware instancing is supported.
    /// @property
    bool GetInstancingSupport() const { return instancingSupport_; }

    /// Return whether light pre-pass rendering is supported.
    /// @property
    bool GetLightPrepassSupport() const { return lightPrepassSupport_; }

    /// Return whether deferred rendering is supported.
    /// @property
    bool GetDeferredSupport() const { return deferredSupport_; }

    /// Return whether anisotropic texture filtering is supported.
    bool GetAnisotropySupport() const { return anisotropySupport_; }

    /// Return whether shadow map depth compare is done in hardware.
    /// @property
    bool GetHardwareShadowSupport() const { return hardwareShadowSupport_; }

    /// Return whether a readable hardware depth format is available.
    /// @property
    bool GetReadableDepthSupport() const { return GetReadableDepthFormat() != 0; }

    /// Return whether sRGB conversion on texture sampling is supported.
    /// @property
    bool GetSRGBSupport() const { return sRGBSupport_; }

    /// Return whether sRGB conversion on rendertarget writing is supported.
    /// @property
    bool GetSRGBWriteSupport() const { return sRGBWriteSupport_; }

    /// Return whether compute shaders are supported.
    bool GetComputeSupport() const { return computeSupport_; }

    /// Return supported fullscreen resolutions (third component is refreshRate). Will be empty if listing the resolutions is not supported on the platform (e.g. Web).
    /// @property
    ea::vector<IntVector3> GetResolutions(int monitor) const;
    /// Return index of the best resolution for requested width, height and refresh rate.
    unsigned FindBestResolutionIndex(int monitor, int width, int height, int refreshRate) const;
    /// Return supported multisampling levels.
    /// @property
    ea::vector<int> GetMultiSampleLevels() const;
    /// Return the desktop resolution.
    /// @property
    IntVector2 GetDesktopResolution(int monitor) const;
    /// Return the number of currently connected monitors.
    /// @property
    int GetMonitorCount() const;
    /// Returns the index of the display containing the center of the window on success or a negative error code on failure.
    /// @property
    int GetCurrentMonitor() const;
    /// Returns true if window is maximized or runs in full screen mode.
    /// @property
    bool GetMaximized() const;
    /// Return display dpi information: (hdpi, vdpi, ddpi). On failure returns zero vector.
    /// @property
    Vector3 GetDisplayDPI(int monitor=0) const;

    /// Return hardware format for a compressed image format, or 0 if unsupported.
    unsigned GetFormat(CompressedFormat format) const;
    /// Return a shader variation by name and defines.
    ShaderVariation* GetShader(ShaderType type, const ea::string& name, const ea::string& defines = EMPTY_STRING) const;
    /// Return a shader variation by name and defines.
    ShaderVariation* GetShader(ShaderType type, const char* name, const char* defines) const;
    /// Return current vertex buffer by index.
    VertexBuffer* GetVertexBuffer(unsigned index) const;

    /// Return current index buffer.
    IndexBuffer* GetIndexBuffer() const { return indexBuffer_; }

    /// Return current vertex shader.
    ShaderVariation* GetVertexShader() const { return vertexShader_; }

    /// Return current pixel shader.
    ShaderVariation* GetPixelShader() const { return pixelShader_; }

    /// Return shader program. This is an API-specific class and should not be used by applications.
    /// @nobind
    ShaderProgram* GetShaderProgram() const;

    /// Return texture unit index by name.
    TextureUnit GetTextureUnit(const ea::string& name);
    /// Return texture unit name by index.
    const ea::string& GetTextureUnitName(TextureUnit unit);
    /// Return current texture by texture unit index.
    Texture* GetTexture(unsigned index) const;

    /// Return default texture filtering mode.
    TextureFilterMode GetDefaultTextureFilterMode() const { return defaultTextureFilterMode_; }

    /// Return default texture max. anisotropy level.
    unsigned GetDefaultTextureAnisotropy() const { return defaultTextureAnisotropy_; }

    /// Return current rendertarget by index.
    RenderSurface* GetRenderTarget(unsigned index) const;

    /// Return current depth-stencil surface.
    RenderSurface* GetDepthStencil() const { return depthStencil_; }

    /// Return the viewport coordinates.
    IntRect GetViewport() const { return viewport_; }

    /// Return blending mode.
    BlendMode GetBlendMode() const { return blendMode_; }

    /// Return whether alpha-to-coverage is enabled.
    bool GetAlphaToCoverage() const { return alphaToCoverage_; }

    /// Return whether color write is enabled.
    bool GetColorWrite() const { return colorWrite_; }

    /// Return hardware culling mode.
    CullMode GetCullMode() const { return cullMode_; }

    /// Return depth constant bias.
    float GetDepthConstantBias() const { return constantDepthBias_; }

    /// Return depth slope scaled bias.
    float GetDepthSlopeScaledBias() const { return slopeScaledDepthBias_; }

    /// Return depth compare mode.
    CompareMode GetDepthTest() const { return depthTestMode_; }

    /// Return whether depth write is enabled.
    bool GetDepthWrite() const { return depthWrite_; }

    /// Return polygon fill mode.
    FillMode GetFillMode() const { return fillMode_; }

    /// Return whether line antialiasing is enabled.
    bool GetLineAntiAlias() const { return lineAntiAlias_; }

    /// Return whether stencil test is enabled.
    bool GetStencilTest() const { return stencilTest_; }

    /// Return whether scissor test is enabled.
    bool GetScissorTest() const { return scissorTest_; }

    /// Return scissor rectangle coordinates.
    const IntRect& GetScissorRect() const { return scissorRect_; }

    /// Return stencil compare mode.
    CompareMode GetStencilTestMode() const { return stencilTestMode_; }

    /// Return stencil operation to do if stencil test passes.
    StencilOp GetStencilPass() const { return stencilPass_; }

    /// Return stencil operation to do if stencil test fails.
    StencilOp GetStencilFail() const { return stencilFail_; }

    /// Return stencil operation to do if depth compare fails.
    StencilOp GetStencilZFail() const { return stencilZFail_; }

    /// Return stencil reference value.
    unsigned GetStencilRef() const { return stencilRef_; }

    /// Return stencil compare bitmask.
    unsigned GetStencilCompareMask() const { return stencilCompareMask_; }

    /// Return stencil write bitmask.
    unsigned GetStencilWriteMask() const { return stencilWriteMask_; }

    /// Return whether a custom clipping plane is in use.
    bool GetUseClipPlane() const { return useClipPlane_; }

    /// Return shader cache directory, Direct3D only.
    /// @property
    const FileIdentifier& GetShaderCacheDir() const { return shaderCacheDir_; }

    /// Return global shader defines.
    const ea::string& GetGlobalShaderDefines() const { return globalShaderDefines_; }

    /// Return global shader defines hash.
    StringHash GetGlobalShaderDefinesHash() const { return globalShaderDefinesHash_; }

    /// Return current rendertarget width and height.
    IntVector2 GetRenderTargetDimensions() const;

    /// Window was resized through user interaction. Called by Input subsystem.
    void OnWindowResized();
    /// Window was moved through user interaction. Called by Input subsystem.
    void OnWindowMoved();
    /// Restore GPU objects and reinitialize state. Requires an open window. Used only on OpenGL.
    /// @nobind
    void Restore();
    /// Maximize the window.
    void Maximize();
    /// Minimize the window.
    void Minimize();
    /// Raises window if it was minimized.
    void Raise() const;
    /// Add a GPU object to keep track of. Called by GPUObject.
    void AddGPUObject(GPUObject* object);
    /// Remove a GPU object. Called by GPUObject.
    void RemoveGPUObject(GPUObject* object);
    /// Reserve a CPU-side scratch buffer.
    void* ReserveScratchBuffer(unsigned size);
    /// Free a CPU-side scratch buffer.
    void FreeScratchBuffer(void* buffer);
    /// Clean up too large scratch buffers.
    void CleanupScratchBuffers();
    /// Clean up shader parameters when a shader variation is released or destroyed.
    void CleanupShaderPrograms(ShaderVariation* variation);
    /// Clean up a render surface from all FBOs. Used only on OpenGL.
    /// @nobind
    void CleanupRenderSurface(RenderSurface* surface);
    /// Get or create a constant buffer. Will be shared between shaders if possible.
    /// @nobind
    ConstantBuffer* GetOrCreateConstantBuffer(ShaderType type, unsigned index, unsigned size);
    /// Mark the FBO needing an update. Used only on OpenGL.
    /// @nobind
    void MarkFBODirty();
    /// Bind a VBO, avoiding redundant operation. Used only on OpenGL.
    /// @nobind
    void SetVBO(unsigned object);
    /// Bind a UBO, avoiding redundant operation. Used only on OpenGL.
    /// @nobind
    void SetUBO(unsigned object);

    /// Return the API-specific alpha texture format.
    static unsigned GetAlphaFormat();
    /// Return the API-specific luminance texture format.
    static unsigned GetLuminanceFormat();
    /// Return the API-specific luminance alpha texture format.
    static unsigned GetLuminanceAlphaFormat();
    /// Return the API-specific RGB texture format.
    static unsigned GetRGBFormat();
    /// Return the API-specific RGBA texture format.
    static unsigned GetRGBAFormat();
    /// Return the API-specific RGBA 16-bit texture format.
    static unsigned GetRGBA16Format();
    /// Return the API-specific RGBA 16-bit float texture format.
    static unsigned GetRGBAFloat16Format();
    /// Return the API-specific RGBA 32-bit float texture format.
    static unsigned GetRGBAFloat32Format();
    /// Return the API-specific RG 16-bit texture format.
    static unsigned GetRG16Format();
    /// Return the API-specific RG 16-bit float texture format.
    static unsigned GetRGFloat16Format();
    /// Return the API-specific RG 32-bit float texture format.
    static unsigned GetRGFloat32Format();
    /// Return the API-specific single channel 16-bit float texture format.
    static unsigned GetFloat16Format();
    /// Return the API-specific single channel 32-bit float texture format.
    static unsigned GetFloat32Format();
    /// Return the API-specific linear depth texture format.
    static unsigned GetLinearDepthFormat();
    /// Return the API-specific hardware depth-stencil texture format.
    static unsigned GetDepthStencilFormat();
    /// Return the API-specific readable hardware depth format, or 0 if not supported.
    static unsigned GetReadableDepthFormat();
    /// Return the API-specific readable hardware depth-stencil format, or 0 if not supported.
    static unsigned GetReadableDepthStencilFormat();
    /// Return the API-specific texture format from a textual description, for example "rgb".
    static unsigned GetFormat(const ea::string& formatName);

    /// Return maximum number of supported bones for skinning.
    static unsigned GetMaxBones();
    /// Return whether is using an OpenGL 3 context. Return always false on DirectX 11.
    static bool GetGL3Support();
    /// Return graphics capabilities.
    static const GraphicsCaps& GetCaps() { return caps; }

    /// Get the SDL_Window as a void* to avoid having to include the graphics implementation
    void* GetSDLWindow() { return window_; }

    void SetLogShaderSources(bool enable) { logShaderSources_ = enable; }
    bool GetLogShaderSources() const { return logShaderSources_; }
    void SetPolicyGLSL(ShaderTranslationPolicy policy) { policyGlsl_ = policy; }
    ShaderTranslationPolicy GetPolicyGLSL() const { return policyGlsl_; }
    void SetPolicyHLSL(ShaderTranslationPolicy policy) { policyHlsl_ = policy; }
    ShaderTranslationPolicy GetPolicyHLSL() const { return policyHlsl_; }

private:
    /// Create the application window.
    bool OpenWindow(int width, int height, bool resizable, bool borderless);
    /// Create the application window icon.
    void CreateWindowIcon();
    /// Adjust parameters according to the platform. Fill in missing paramters and resolve possible conflicts.
    void AdjustScreenMode(int& newWidth, int& newHeight, ScreenModeParams& params, bool& maximize) const;
    /// Called when screen mode is successfully changed by the backend.
    void OnScreenModeChanged();
    /// Adjust the window for new resolution and fullscreen mode.
    void AdjustWindow(int& newWidth, int& newHeight, WindowMode& newWindowMode, int& monitor);
    /// Create the Direct3D11 device and swap chain. Requires an open window. Can also be called again to recreate swap chain. Return true on success.
    bool CreateDevice(int width, int height);
    /// Update Direct3D11 swap chain state for a new mode and create views for the backbuffer & default depth buffer. Return true on success.
    bool UpdateSwapChain(int width, int height);
    /// Check supported rendering features.
    void CheckFeatureSupport();
    /// Reset cached rendering state.
    void ResetCachedState();
    /// Initialize texture unit mappings.
    void SetTextureUnitMappings();
    /// Process dirtied state before draw.
    void PrepareDraw();
    /// Create intermediate texture for multisampled backbuffer resolve. No-op if already exists.
    void CreateResolveTexture();
    /// Clean up all framebuffers. Called when destroying the context. Used only on OpenGL.
    void CleanupFramebuffers();
    /// Create a framebuffer using either extension or core functionality. Used only on OpenGL.
    unsigned CreateFramebuffer();
    /// Delete a framebuffer using either extension or core functionality. Used only on OpenGL.
    void DeleteFramebuffer(unsigned fbo);
    /// Bind a framebuffer using either extension or core functionality. Used only on OpenGL.
    void BindFramebuffer(unsigned fbo);
    /// Bind a framebuffer color attachment using either extension or core functionality. Used only on OpenGL.
    void BindColorAttachment(unsigned index, unsigned target, unsigned object, bool isRenderBuffer);
    /// Bind a framebuffer depth attachment using either extension or core functionality. Used only on OpenGL.
    void BindDepthAttachment(unsigned object, bool isRenderBuffer);
    /// Bind a framebuffer stencil attachment using either extension or core functionality. Used only on OpenGL.
    void BindStencilAttachment(unsigned object, bool isRenderBuffer);
    /// Check FBO completeness using either extension or core functionality. Used only on OpenGL.
    bool CheckFramebuffer();
    /// Set vertex attrib divisor. No-op if unsupported. Used only on OpenGL.
    void SetVertexAttribDivisor(unsigned location, unsigned divisor);
    /// Release/clear GPU objects and optionally close the window. Used only on OpenGL.
    void Release(bool clearGPUObjects, bool closeWindow);

    /// Mutex for accessing the GPU objects vector from several threads.
    Mutex gpuObjectMutex_;
    /// Implementation.
    GraphicsImpl* impl_;
    /// SDL window.
    SDL_Window* window_{};
    /// Window title.
    ea::string windowTitle_;
    /// Window icon image.
    WeakPtr<Image> windowIcon_;
    /// External window, null if not in use (default.)
    void* externalWindow_{};
    /// Most recently applied window mode. It may not represent actual window state
    /// if window was resized by user or Graphics::SetScreenMode was explicitly called.
    WindowModeParams primaryWindowMode_;
    /// Secondary window mode to be applied on Graphics::ToggleFullscreen.
    WindowModeParams secondaryWindowMode_;
    /// Window width in pixels.
    int width_{};
    /// Window height in pixels.
    int height_{};
    /// Window position.
    IntVector2 position_;
    /// Screen mode parameters.
    ScreenModeParams screenParams_;
    /// Whether the shader validation is enabled.
    bool validateShaders_{};
    /// Flush GPU command buffer flag.
    bool flushGPU_{};
    /// Force OpenGL 2 flag. Only used on OpenGL.
    bool forceGL2_{};
    /// sRGB conversion on write flag for the main window.
    bool sRGB_{};
    /// Light pre-pass rendering support flag.
    bool lightPrepassSupport_{};
    /// Deferred rendering support flag.
    bool deferredSupport_{};
    /// Anisotropic filtering support flag.
    bool anisotropySupport_{};
    /// DXT format support flag.
    bool dxtTextureSupport_{};
    /// ETC1 format support flag.
    bool etcTextureSupport_{};
    /// ETC2 format support flag.
    bool etc2TextureSupport_{};
    /// PVRTC formats support flag.
    bool pvrtcTextureSupport_{};
    /// Hardware shadow map depth compare support flag.
    bool hardwareShadowSupport_{};
    /// Instancing support flag.
    bool instancingSupport_{};
    /// sRGB conversion on read support flag.
    bool sRGBSupport_{};
    /// sRGB conversion on write support flag.
    bool sRGBWriteSupport_{};
    /// Compute shaders support.
    bool computeSupport_{};
    /// Number of primitives this frame.
    unsigned numPrimitives_{};
    /// Number of batches this frame.
    unsigned numBatches_{};
    /// Largest scratch buffer request this frame.
    unsigned maxScratchBufferRequest_{};
    /// GPU objects.
    ea::vector<GPUObject*> gpuObjects_;
    /// Scratch buffers.
    ea::vector<ScratchBuffer> scratchBuffers_;
    /// Shadow map dummy color texture format.
    unsigned dummyColorFormat_{};
    /// Shadow map depth texture format.
    unsigned shadowMapFormat_{};
    /// Shadow map 24-bit depth texture format.
    unsigned hiresShadowMapFormat_{};
    /// Vertex buffers in use.
    VertexBuffer* vertexBuffers_[MAX_VERTEX_STREAMS]{};
    /// Index buffer in use.
    IndexBuffer* indexBuffer_{};
    /// Current vertex declaration hash.
    unsigned long long vertexDeclarationHash_{};
    /// Current primitive type.
    unsigned primitiveType_{};
    /// Vertex shader in use.
    ShaderVariation* vertexShader_{};
    /// Pixel shader in use.
    ShaderVariation* pixelShader_{};
    /// Textures in use.
    Texture* textures_[MAX_TEXTURE_UNITS]{};
    /// Texture unit mappings.
    ea::unordered_map<ea::string, TextureUnit> textureUnits_;
    /// Rendertargets in use.
    RenderSurface* renderTargets_[MAX_RENDERTARGETS]{};
    /// Constan buffer ranges in use.
    ConstantBufferRange constantBuffers_[MAX_SHADER_PARAMETER_GROUPS]{};
    /// Depth-stencil surface in use.
    RenderSurface* depthStencil_{};
    /// Viewport coordinates.
    IntRect viewport_;
    /// Default texture filtering mode.
    TextureFilterMode defaultTextureFilterMode_{FILTER_TRILINEAR};
    /// Default texture max. anisotropy level.
    unsigned defaultTextureAnisotropy_{4};
    /// Blending mode.
    BlendMode blendMode_{};
    /// Alpha-to-coverage enable.
    bool alphaToCoverage_{};
    /// Color write enable.
    bool colorWrite_{};
    /// Hardware culling mode.
    CullMode cullMode_{};
    /// Depth constant bias.
    float constantDepthBias_{};
    /// Depth slope scaled bias.
    float slopeScaledDepthBias_{};
    /// Depth compare mode.
    CompareMode depthTestMode_{};
    /// Depth write enable flag.
    bool depthWrite_{};
    /// Line antialiasing enable flag.
    bool lineAntiAlias_{};
    /// Polygon fill mode.
    FillMode fillMode_{};
    /// Scissor test enable flag.
    bool scissorTest_{};
    /// Scissor test rectangle.
    IntRect scissorRect_;
    /// Stencil test compare mode.
    CompareMode stencilTestMode_{};
    /// Stencil operation on pass.
    StencilOp stencilPass_{};
    /// Stencil operation on fail.
    StencilOp stencilFail_{};
    /// Stencil operation on depth fail.
    StencilOp stencilZFail_{};
    /// Stencil test reference value.
    unsigned stencilRef_{};
    /// Stencil compare bitmask.
    unsigned stencilCompareMask_{};
    /// Stencil write bitmask.
    unsigned stencilWriteMask_{};
    /// Current custom clip plane in post-projection space.
    Vector4 clipPlane_;
    /// Stencil test enable flag.
    bool stencilTest_{};
    /// Custom clip plane enable flag.
    bool useClipPlane_{};
    /// Remembered shader parameter sources.
    const void* shaderParameterSources_[MAX_SHADER_PARAMETER_GROUPS]{};
    /// Base directory for shaders.
    ea::string shaderPath_;
    /// Shader name prefix for universal shaders.
    ea::string universalShaderNamePrefix_{ "v2/" };
    /// Format string for universal shaders.
    ea::string universalShaderPath_{ "Shaders/GLSL/{}.glsl" };
    /// Cache directory for Direct3D binary shaders.
    FileIdentifier shaderCacheDir_;
    /// File extension for shaders.
    ea::string shaderExtension_;
    /// Last used shader in shader variation query.
    mutable WeakPtr<Shader> lastShader_;
    /// Last used shader name in shader variation query.
    mutable ea::string lastShaderName_;
    /// Shader precache utility.
    SharedPtr<ShaderPrecache> shaderPrecache_;
    /// Allowed screen orientations.
    ea::string orientations_;
    /// Graphics API name.
    ea::string apiName_;
    /// Global shader defines.
    ea::string globalShaderDefines_;
    /// Hash of global shader defines.
    StringHash globalShaderDefinesHash_;

    bool logShaderSources_{};
    ShaderTranslationPolicy policyGlsl_{ShaderTranslationPolicy::Verbatim};
    ShaderTranslationPolicy policyHlsl_{ShaderTranslationPolicy::Translate};

    /// OpenGL3 support flag.
    static bool gl3Support;
    /// Graphics capabilities. Static for easier access.
    static GraphicsCaps caps;
};

/// Register Graphics library objects.
/// @nobind
void URHO3D_API RegisterGraphicsLibrary(Context* context);

}

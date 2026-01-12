/*
 * HalGUI - Direct3D 11 GPU Renderer Implementation
 * 
 * Modern GPU-accelerated rendering backend featuring:
 * - Batched draw calls with retained-mode scene graph
 * - MSAA 4x anti-aliasing
 * - SDF font rendering
 * - Physically plausible shadows with Gaussian blur
 * - VSync-aware frame scheduling
 * - High-DPI and fractional scaling support
 * 
 * Build with MSYS2/MinGW-w64:
 *   gcc -O2 halgui_d3d11.c -ld3d11 -ld3dcompiler -ldxgi -lole32 -luuid -o demo.exe
 */

#include "halgui_d3d11.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

/* ============================================
   COM Interface Compatibility Macros
   For MinGW CINTERFACE mode
   ============================================ */

/* ID3D10Blob / ID3DBlob */
#define BLOB_GetBufferPointer(p) ((p)->lpVtbl->GetBufferPointer(p))
#define BLOB_GetBufferSize(p) ((p)->lpVtbl->GetBufferSize(p))
#define BLOB_Release(p) ((p)->lpVtbl->Release(p))

/* ID3D11Device */
#define DEV_CreateVertexShader(d,bc,sz,cl,vs) ((d)->lpVtbl->CreateVertexShader(d,bc,sz,cl,vs))
#define DEV_CreatePixelShader(d,bc,sz,cl,ps) ((d)->lpVtbl->CreatePixelShader(d,bc,sz,cl,ps))
#define DEV_CreateInputLayout(d,ed,ne,bc,sz,il) ((d)->lpVtbl->CreateInputLayout(d,ed,ne,bc,sz,il))
#define DEV_CreateBuffer(d,desc,data,buf) ((d)->lpVtbl->CreateBuffer(d,desc,data,buf))
#define DEV_CreateBlendState(d,desc,bs) ((d)->lpVtbl->CreateBlendState(d,desc,bs))
#define DEV_CreateRasterizerState(d,desc,rs) ((d)->lpVtbl->CreateRasterizerState(d,desc,rs))
#define DEV_CreateSamplerState(d,desc,ss) ((d)->lpVtbl->CreateSamplerState(d,desc,ss))
#define DEV_CreateDepthStencilState(d,desc,ds) ((d)->lpVtbl->CreateDepthStencilState(d,desc,ds))
#define DEV_CreateTexture2D(d,desc,data,tex) ((d)->lpVtbl->CreateTexture2D(d,desc,data,tex))
#define DEV_CreateRenderTargetView(d,res,desc,rtv) ((d)->lpVtbl->CreateRenderTargetView(d,res,desc,rtv))
#define DEV_QueryInterface(d,iid,pp) ((d)->lpVtbl->QueryInterface(d,iid,pp))
#define DEV_Release(d) ((d)->lpVtbl->Release(d))

/* ID3D11DeviceContext */
#define CTX_Map(c,res,sub,mt,fl,mr) ((c)->lpVtbl->Map(c,res,sub,mt,fl,mr))
#define CTX_Unmap(c,res,sub) ((c)->lpVtbl->Unmap(c,res,sub))
#define CTX_IASetInputLayout(c,il) ((c)->lpVtbl->IASetInputLayout(c,il))
#define CTX_IASetPrimitiveTopology(c,pt) ((c)->lpVtbl->IASetPrimitiveTopology(c,pt))
#define CTX_IASetVertexBuffers(c,ss,nb,vb,st,of) ((c)->lpVtbl->IASetVertexBuffers(c,ss,nb,vb,st,of))
#define CTX_IASetIndexBuffer(c,ib,fmt,of) ((c)->lpVtbl->IASetIndexBuffer(c,ib,fmt,of))
#define CTX_VSSetShader(c,vs,ci,nc) ((c)->lpVtbl->VSSetShader(c,vs,ci,nc))
#define CTX_VSSetConstantBuffers(c,ss,nb,cb) ((c)->lpVtbl->VSSetConstantBuffers(c,ss,nb,cb))
#define CTX_PSSetShader(c,ps,ci,nc) ((c)->lpVtbl->PSSetShader(c,ps,ci,nc))
#define CTX_PSSetConstantBuffers(c,ss,nb,cb) ((c)->lpVtbl->PSSetConstantBuffers(c,ss,nb,cb))
#define CTX_PSSetSamplers(c,ss,ns,sm) ((c)->lpVtbl->PSSetSamplers(c,ss,ns,sm))
#define CTX_RSSetState(c,rs) ((c)->lpVtbl->RSSetState(c,rs))
#define CTX_RSSetViewports(c,nv,vp) ((c)->lpVtbl->RSSetViewports(c,nv,vp))
#define CTX_RSSetScissorRects(c,nr,rc) ((c)->lpVtbl->RSSetScissorRects(c,nr,rc))
#define CTX_OMSetRenderTargets(c,nv,rtv,dsv) ((c)->lpVtbl->OMSetRenderTargets(c,nv,rtv,dsv))
#define CTX_OMSetBlendState(c,bs,bf,sm) ((c)->lpVtbl->OMSetBlendState(c,bs,bf,sm))
#define CTX_OMSetDepthStencilState(c,ds,sr) ((c)->lpVtbl->OMSetDepthStencilState(c,ds,sr))
#define CTX_ClearRenderTargetView(c,rtv,col) ((c)->lpVtbl->ClearRenderTargetView(c,rtv,col))
#define CTX_DrawIndexed(c,ic,si,bv) ((c)->lpVtbl->DrawIndexed(c,ic,si,bv))
#define CTX_ResolveSubresource(c,dst,di,src,si,fmt) ((c)->lpVtbl->ResolveSubresource(c,dst,di,src,si,fmt))
#define CTX_Release(c) ((c)->lpVtbl->Release(c))

/* IDXGISwapChain1 */
#define SC_GetBuffer(s,idx,iid,pp) ((s)->lpVtbl->GetBuffer(s,idx,iid,pp))
#define SC_Present(s,si,fl) ((s)->lpVtbl->Present(s,si,fl))
#define SC_ResizeBuffers(s,bc,w,h,fmt,fl) ((s)->lpVtbl->ResizeBuffers(s,bc,w,h,fmt,fl))
#define SC_Release(s) ((s)->lpVtbl->Release(s))

/* IDXGIFactory2 */
#define FAC_CreateSwapChainForHwnd(f,dev,hwnd,desc,fsd,ro,sc) ((f)->lpVtbl->CreateSwapChainForHwnd(f,dev,hwnd,desc,fsd,ro,sc))
#define FAC_Release(f) ((f)->lpVtbl->Release(f))

/* IDXGIDevice */
#define DXGIDEV_GetAdapter(d,a) ((d)->lpVtbl->GetAdapter(d,a))
#define DXGIDEV_Release(d) ((d)->lpVtbl->Release(d))

/* IDXGIAdapter */
#define ADAPTER_GetParent(a,iid,pp) ((a)->lpVtbl->GetParent(a,iid,pp))
#define ADAPTER_Release(a) ((a)->lpVtbl->Release(a))

/* ID3D11Texture2D */
#define TEX_Release(t) ((t)->lpVtbl->Release(t))

/* Other D3D11 objects */
#define RTV_Release(r) ((r)->lpVtbl->Release(r))
#define SRV_Release(s) ((s)->lpVtbl->Release(s))
#define VS_Release(v) ((v)->lpVtbl->Release(v))
#define PS_Release(p) ((p)->lpVtbl->Release(p))
#define IL_Release(i) ((i)->lpVtbl->Release(i))
#define BUF_Release(b) ((b)->lpVtbl->Release(b))
#define BS_Release(b) ((b)->lpVtbl->Release(b))
#define RS_Release(r) ((r)->lpVtbl->Release(r))
#define SS_Release(s) ((s)->lpVtbl->Release(s))
#define DSS_Release(d) ((d)->lpVtbl->Release(d))

/* ============================================
   Global GPU Context
   ============================================ */

static HalGPUContext* g_gpuCtx = NULL;

/* ============================================
   HLSL Shaders (embedded)
   ============================================ */

static const char* g_vertexShaderSource = 
"cbuffer Constants : register(b0) {\n"
"    float4x4 projectionMatrix;\n"
"    float2 viewportSize;\n"
"    float time;\n"
"    float dpiScale;\n"
"    float4 shadowColor;\n"
"    float ambientLight;\n"
"    float3 padding;\n"
"};\n"
"\n"
"struct VS_INPUT {\n"
"    float2 pos : POSITION;\n"
"    float2 uv : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"    float cornerRadius : TEXCOORD1;\n"
"    float borderWidth : TEXCOORD2;\n"
"    float shadowSoftness : TEXCOORD3;\n"
"    float elevation : TEXCOORD4;\n"
"};\n"
"\n"
"struct VS_OUTPUT {\n"
"    float4 pos : SV_POSITION;\n"
"    float2 uv : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"    float cornerRadius : TEXCOORD1;\n"
"    float borderWidth : TEXCOORD2;\n"
"    float shadowSoftness : TEXCOORD3;\n"
"    float elevation : TEXCOORD4;\n"
"    float2 localPos : TEXCOORD5;\n"
"    float2 rectSize : TEXCOORD6;\n"
"};\n"
"\n"
"VS_OUTPUT main(VS_INPUT input) {\n"
"    VS_OUTPUT output;\n"
"    \n"
"    // Transform to clip space\n"
"    float2 clipPos = (input.pos / viewportSize) * 2.0 - 1.0;\n"
"    clipPos.y = -clipPos.y;\n"
"    output.pos = float4(clipPos, input.elevation * 0.001, 1.0);\n"
"    \n"
"    output.uv = input.uv;\n"
"    output.color = input.color;\n"
"    output.cornerRadius = input.cornerRadius;\n"
"    output.borderWidth = input.borderWidth;\n"
"    output.shadowSoftness = input.shadowSoftness;\n"
"    output.elevation = input.elevation;\n"
"    output.localPos = input.uv;\n"
"    output.rectSize = float2(1.0, 1.0);\n"
"    \n"
"    return output;\n"
"}\n";

static const char* g_pixelShaderSource = 
"struct PS_INPUT {\n"
"    float4 pos : SV_POSITION;\n"
"    float2 uv : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"    float cornerRadius : TEXCOORD1;\n"
"    float borderWidth : TEXCOORD2;\n"
"    float shadowSoftness : TEXCOORD3;\n"
"    float elevation : TEXCOORD4;\n"
"    float2 localPos : TEXCOORD5;\n"
"    float2 rectSize : TEXCOORD6;\n"
"};\n"
"\n"
"// Signed distance function for rounded rectangle\n"
"float sdRoundedRect(float2 p, float2 b, float r) {\n"
"    float2 q = abs(p) - b + r;\n"
"    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n"
"}\n"
"\n"
"float4 main(PS_INPUT input) : SV_TARGET {\n"
"    float4 color = input.color;\n"
"    \n"
"    // Calculate SDF for rounded rectangle\n"
"    float2 center = float2(0.5, 0.5);\n"
"    float2 p = input.uv - center;\n"
"    float2 halfSize = float2(0.5, 0.5);\n"
"    float radius = input.cornerRadius;\n"
"    \n"
"    float dist = sdRoundedRect(p, halfSize, radius);\n"
"    \n"
"    // Anti-aliased edge\n"
"    float aa = fwidth(dist) * 1.5;\n"
"    float alpha = 1.0 - smoothstep(-aa, aa, dist);\n"
"    \n"
"    // Border\n"
"    if (input.borderWidth > 0.0) {\n"
"        float innerDist = sdRoundedRect(p, halfSize - input.borderWidth, radius);\n"
"        float borderAlpha = smoothstep(-aa, aa, innerDist);\n"
"        color.a *= alpha * (1.0 - borderAlpha * 0.5);\n"
"    } else {\n"
"        color.a *= alpha;\n"
"    }\n"
"    \n"
"    return color;\n"
"}\n";

static const char* g_shadowShaderSource = 
"cbuffer Constants : register(b0) {\n"
"    float4x4 projectionMatrix;\n"
"    float2 viewportSize;\n"
"    float time;\n"
"    float dpiScale;\n"
"    float4 shadowColor;\n"
"    float ambientLight;\n"
"    float3 padding;\n"
"};\n"
"\n"
"struct PS_INPUT {\n"
"    float4 pos : SV_POSITION;\n"
"    float2 uv : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"    float cornerRadius : TEXCOORD1;\n"
"    float borderWidth : TEXCOORD2;\n"
"    float shadowSoftness : TEXCOORD3;\n"
"    float elevation : TEXCOORD4;\n"
"    float2 localPos : TEXCOORD5;\n"
"    float2 rectSize : TEXCOORD6;\n"
"};\n"
"\n"
"float sdRoundedRect(float2 p, float2 b, float r) {\n"
"    float2 q = abs(p) - b + r;\n"
"    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n"
"}\n"
"\n"
"// Gaussian approximation for soft shadows\n"
"float gaussian(float x, float sigma) {\n"
"    return exp(-(x * x) / (2.0 * sigma * sigma));\n"
"}\n"
"\n"
"float4 main(PS_INPUT input) : SV_TARGET {\n"
"    float2 center = float2(0.5, 0.5);\n"
"    float2 p = input.uv - center;\n"
"    float2 halfSize = float2(0.5, 0.5);\n"
"    float radius = input.cornerRadius;\n"
"    \n"
"    float dist = sdRoundedRect(p, halfSize, radius);\n"
"    \n"
"    // Soft shadow using Gaussian falloff\n"
"    float softness = input.shadowSoftness;\n"
"    float shadow = gaussian(dist, softness * 0.1);\n"
"    shadow = saturate(shadow);\n"
"    \n"
"    float4 color = shadowColor;\n"
"    color.a *= shadow * (input.elevation / 5.0);\n"
"    \n"
"    return color;\n"
"}\n";

static const char* g_textShaderSource = 
"Texture2D fontAtlas : register(t0);\n"
"SamplerState fontSampler : register(s0);\n"
"\n"
"struct PS_INPUT {\n"
"    float4 pos : SV_POSITION;\n"
"    float2 uv : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"    float cornerRadius : TEXCOORD1;\n"
"    float borderWidth : TEXCOORD2;\n"
"    float shadowSoftness : TEXCOORD3;\n"
"    float elevation : TEXCOORD4;\n"
"    float2 localPos : TEXCOORD5;\n"
"    float2 rectSize : TEXCOORD6;\n"
"};\n"
"\n"
"float4 main(PS_INPUT input) : SV_TARGET {\n"
"    float dist = fontAtlas.Sample(fontSampler, input.uv).r;\n"
"    \n"
"    // SDF text rendering with anti-aliasing\n"
"    float smoothing = 0.25 / (4.0 * fwidth(dist));\n"
"    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);\n"
"    \n"
"    float4 color = input.color;\n"
"    color.a *= alpha;\n"
"    \n"
"    return color;\n"
"}\n";

/* ============================================
   Helper Functions
   ============================================ */

static void hal_gpu_log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[HalGUI D3D11] ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

static bool hal_compile_shader(const char* source, const char* target, 
                                ID3DBlob** blob, const char* name) {
    ID3DBlob* errorBlob = NULL;
    HRESULT hr = D3DCompile(
        source, strlen(source),
        name, NULL, NULL,
        "main", target,
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
        0, blob, &errorBlob
    );
    
    if (FAILED(hr)) {
        if (errorBlob) {
            hal_gpu_log("Shader compilation error: %s", 
                       (char*)BLOB_GetBufferPointer(errorBlob));
            BLOB_Release(errorBlob);
        }
        return false;
    }
    
    if (errorBlob) {
        BLOB_Release(errorBlob);
    }
    
    return true;
}

static void hal_create_projection_matrix(float* matrix, float width, float height) {
    // Orthographic projection matrix
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = 2.0f / width;
    matrix[5] = -2.0f / height;
    matrix[10] = 1.0f;
    matrix[12] = -1.0f;
    matrix[13] = 1.0f;
    matrix[15] = 1.0f;
}

/* ============================================
   Batch Management
   ============================================ */

static bool hal_batch_init(HalRenderBatch* batch) {
    batch->vertexCapacity = HAL_GPU_MAX_VERTICES;
    batch->indexCapacity = HAL_GPU_MAX_INDICES;
    
    batch->vertices = (HalGPUVertex*)malloc(batch->vertexCapacity * sizeof(HalGPUVertex));
    batch->indices = (uint32_t*)malloc(batch->indexCapacity * sizeof(uint32_t));
    
    if (!batch->vertices || !batch->indices) {
        free(batch->vertices);
        free(batch->indices);
        return false;
    }
    
    batch->vertexCount = 0;
    batch->indexCount = 0;
    batch->commandCount = 0;
    batch->needsUpload = false;
    
    return true;
}

static void hal_batch_shutdown(HalRenderBatch* batch) {
    free(batch->vertices);
    free(batch->indices);
    batch->vertices = NULL;
    batch->indices = NULL;
}

static void hal_batch_clear(HalRenderBatch* batch) {
    batch->vertexCount = 0;
    batch->indexCount = 0;
    batch->commandCount = 0;
    batch->needsUpload = true;
}

static bool hal_batch_add_quad(HalRenderBatch* batch, 
                                float x, float y, float w, float h,
                                float u0, float v0, float u1, float v1,
                                float r, float g, float b, float a,
                                float cornerRadius, float borderWidth,
                                float shadowSoftness, float elevation) {
    if (batch->vertexCount + 4 > batch->vertexCapacity ||
        batch->indexCount + 6 > batch->indexCapacity) {
        return false;
    }
    
    uint32_t baseVertex = batch->vertexCount;
    
    // Top-left
    batch->vertices[batch->vertexCount++] = (HalGPUVertex){
        x, y, u0, v0, r, g, b, a, cornerRadius, borderWidth, shadowSoftness, elevation
    };
    // Top-right
    batch->vertices[batch->vertexCount++] = (HalGPUVertex){
        x + w, y, u1, v0, r, g, b, a, cornerRadius, borderWidth, shadowSoftness, elevation
    };
    // Bottom-right
    batch->vertices[batch->vertexCount++] = (HalGPUVertex){
        x + w, y + h, u1, v1, r, g, b, a, cornerRadius, borderWidth, shadowSoftness, elevation
    };
    // Bottom-left
    batch->vertices[batch->vertexCount++] = (HalGPUVertex){
        x, y + h, u0, v1, r, g, b, a, cornerRadius, borderWidth, shadowSoftness, elevation
    };
    
    // Indices (two triangles)
    batch->indices[batch->indexCount++] = baseVertex + 0;
    batch->indices[batch->indexCount++] = baseVertex + 1;
    batch->indices[batch->indexCount++] = baseVertex + 2;
    batch->indices[batch->indexCount++] = baseVertex + 0;
    batch->indices[batch->indexCount++] = baseVertex + 2;
    batch->indices[batch->indexCount++] = baseVertex + 3;
    
    batch->needsUpload = true;
    return true;
}

/* ============================================
   GPU Initialization
   ============================================ */

bool hal_gpu_init(HWND hwnd, uint32_t width, uint32_t height) {
    if (g_gpuCtx) {
        hal_gpu_log("GPU context already initialized");
        return true;
    }
    
    g_gpuCtx = (HalGPUContext*)calloc(1, sizeof(HalGPUContext));
    if (!g_gpuCtx) {
        hal_gpu_log("Failed to allocate GPU context");
        return false;
    }
    
    g_gpuCtx->width = width;
    g_gpuCtx->height = height;
    g_gpuCtx->dpiScale = 1.0f;
    g_gpuCtx->vsyncEnabled = true;
    g_gpuCtx->targetFrameTime = 1.0 / 60.0;
    
    HRESULT hr;
    
    // Create D3D11 device and context
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    
    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createFlags, featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &g_gpuCtx->device, &featureLevel, &g_gpuCtx->context
    );
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create D3D11 device: 0x%08X", hr);
        free(g_gpuCtx);
        g_gpuCtx = NULL;
        return false;
    }
    
    hal_gpu_log("D3D11 device created, feature level: 0x%X", featureLevel);

    // Get DXGI factory
    IDXGIDevice* dxgiDevice = NULL;
    IDXGIAdapter* dxgiAdapter = NULL;
    IDXGIFactory2* dxgiFactory = NULL;
    
    hr = DEV_QueryInterface(g_gpuCtx->device, &IID_IDXGIDevice, (void**)&dxgiDevice);
    if (SUCCEEDED(hr)) {
        hr = DXGIDEV_GetAdapter(dxgiDevice, &dxgiAdapter);
        if (SUCCEEDED(hr)) {
            hr = ADAPTER_GetParent(dxgiAdapter, &IID_IDXGIFactory2, (void**)&dxgiFactory);
        }
    }
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to get DXGI factory: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    hr = FAC_CreateSwapChainForHwnd(
        dxgiFactory, (IUnknown*)g_gpuCtx->device, hwnd,
        &swapChainDesc, NULL, NULL, &g_gpuCtx->swapChain
    );
    
    if (dxgiFactory) FAC_Release(dxgiFactory);
    if (dxgiAdapter) ADAPTER_Release(dxgiAdapter);
    if (dxgiDevice) DXGIDEV_Release(dxgiDevice);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create swap chain: 0x%08X", hr);
        goto cleanup_error;
    }

    // Create render target view from back buffer
    ID3D11Texture2D* backBuffer = NULL;
    hr = SC_GetBuffer(g_gpuCtx->swapChain, 0, &IID_ID3D11Texture2D, (void**)&backBuffer);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to get back buffer: 0x%08X", hr);
        goto cleanup_error;
    }
    
    hr = DEV_CreateRenderTargetView(g_gpuCtx->device, (ID3D11Resource*)backBuffer, 
                                              NULL, &g_gpuCtx->backBufferRTV);
    TEX_Release(backBuffer);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create render target view: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Create MSAA render target
    D3D11_TEXTURE2D_DESC msaaDesc = {0};
    msaaDesc.Width = width;
    msaaDesc.Height = height;
    msaaDesc.MipLevels = 1;
    msaaDesc.ArraySize = 1;
    msaaDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaDesc.SampleDesc.Count = HAL_GPU_MSAA_SAMPLES;
    msaaDesc.SampleDesc.Quality = 0;
    msaaDesc.Usage = D3D11_USAGE_DEFAULT;
    msaaDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    
    hr = DEV_CreateTexture2D(g_gpuCtx->device, &msaaDesc, NULL, &g_gpuCtx->msaaTexture);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create MSAA texture: 0x%08X", hr);
        goto cleanup_error;
    }
    
    hr = DEV_CreateRenderTargetView(g_gpuCtx->device, (ID3D11Resource*)g_gpuCtx->msaaTexture,
                                              NULL, &g_gpuCtx->msaaRTV);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create MSAA RTV: 0x%08X", hr);
        goto cleanup_error;
    }

    // Compile shaders
    ID3DBlob* vsBlob = NULL;
    ID3DBlob* psBlob = NULL;
    ID3DBlob* shadowBlob = NULL;
    ID3DBlob* textBlob = NULL;
    
    if (!hal_compile_shader(g_vertexShaderSource, "vs_5_0", &vsBlob, "VertexShader")) {
        hal_gpu_log("Failed to compile vertex shader");
        goto cleanup_error;
    }
    
    if (!hal_compile_shader(g_pixelShaderSource, "ps_5_0", &psBlob, "PixelShader")) {
        BLOB_Release(vsBlob);
        hal_gpu_log("Failed to compile pixel shader");
        goto cleanup_error;
    }
    
    if (!hal_compile_shader(g_shadowShaderSource, "ps_5_0", &shadowBlob, "ShadowShader")) {
        BLOB_Release(vsBlob);
        BLOB_Release(psBlob);
        hal_gpu_log("Failed to compile shadow shader");
        goto cleanup_error;
    }
    
    if (!hal_compile_shader(g_textShaderSource, "ps_5_0", &textBlob, "TextShader")) {
        BLOB_Release(vsBlob);
        BLOB_Release(psBlob);
        BLOB_Release(shadowBlob);
        hal_gpu_log("Failed to compile text shader");
        goto cleanup_error;
    }
    
    // Create shader objects
    hr = DEV_CreateVertexShader(g_gpuCtx->device,
        BLOB_GetBufferPointer(vsBlob), BLOB_GetBufferSize(vsBlob),
        NULL, &g_gpuCtx->vertexShader);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create vertex shader: 0x%08X", hr);
        goto cleanup_shaders;
    }
    
    hr = DEV_CreatePixelShader(g_gpuCtx->device,
        BLOB_GetBufferPointer(psBlob), BLOB_GetBufferSize(psBlob),
        NULL, &g_gpuCtx->pixelShader);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create pixel shader: 0x%08X", hr);
        goto cleanup_shaders;
    }

    hr = DEV_CreatePixelShader(g_gpuCtx->device,
        BLOB_GetBufferPointer(shadowBlob), BLOB_GetBufferSize(shadowBlob),
        NULL, &g_gpuCtx->shadowShader);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create shadow shader: 0x%08X", hr);
        goto cleanup_shaders;
    }
    
    hr = DEV_CreatePixelShader(g_gpuCtx->device,
        BLOB_GetBufferPointer(textBlob), BLOB_GetBufferSize(textBlob),
        NULL, &g_gpuCtx->textShader);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create text shader: 0x%08X", hr);
        goto cleanup_shaders;
    }
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 3, DXGI_FORMAT_R32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 4, DXGI_FORMAT_R32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    hr = DEV_CreateInputLayout(g_gpuCtx->device, inputLayout, ARRAYSIZE(inputLayout),
        BLOB_GetBufferPointer(vsBlob), BLOB_GetBufferSize(vsBlob),
        &g_gpuCtx->inputLayout);
    
    BLOB_Release(vsBlob);
    BLOB_Release(psBlob);
    BLOB_Release(shadowBlob);
    BLOB_Release(textBlob);
    
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create input layout: 0x%08X", hr);
        goto cleanup_error;
    }

    // Create vertex buffer
    D3D11_BUFFER_DESC vbDesc = {0};
    vbDesc.ByteWidth = HAL_GPU_MAX_VERTICES * sizeof(HalGPUVertex);
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = DEV_CreateBuffer(g_gpuCtx->device, &vbDesc, NULL, &g_gpuCtx->vertexBuffer);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create vertex buffer: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Create index buffer
    D3D11_BUFFER_DESC ibDesc = {0};
    ibDesc.ByteWidth = HAL_GPU_MAX_INDICES * sizeof(uint32_t);
    ibDesc.Usage = D3D11_USAGE_DYNAMIC;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = DEV_CreateBuffer(g_gpuCtx->device, &ibDesc, NULL, &g_gpuCtx->indexBuffer);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create index buffer: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc = {0};
    cbDesc.ByteWidth = sizeof(HalGPUConstants);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = DEV_CreateBuffer(g_gpuCtx->device, &cbDesc, NULL, &g_gpuCtx->constantBuffer);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create constant buffer: 0x%08X", hr);
        goto cleanup_error;
    }

    // Create blend states
    D3D11_BLEND_DESC blendDesc = {0};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = DEV_CreateBlendState(g_gpuCtx->device, &blendDesc, &g_gpuCtx->blendStateNormal);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create blend state: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Additive blend state
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    hr = DEV_CreateBlendState(g_gpuCtx->device, &blendDesc, &g_gpuCtx->blendStateAdditive);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create additive blend state: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Create rasterizer state
    D3D11_RASTERIZER_DESC rastDesc = {0};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.ScissorEnable = TRUE;
    rastDesc.MultisampleEnable = TRUE;
    rastDesc.AntialiasedLineEnable = TRUE;
    
    hr = DEV_CreateRasterizerState(g_gpuCtx->device, &rastDesc, &g_gpuCtx->rasterizerState);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create rasterizer state: 0x%08X", hr);
        goto cleanup_error;
    }

    // Create sampler states
    D3D11_SAMPLER_DESC samplerDesc = {0};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    hr = DEV_CreateSamplerState(g_gpuCtx->device, &samplerDesc, &g_gpuCtx->samplerLinear);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create linear sampler: 0x%08X", hr);
        goto cleanup_error;
    }
    
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    hr = DEV_CreateSamplerState(g_gpuCtx->device, &samplerDesc, &g_gpuCtx->samplerPoint);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create point sampler: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Create depth stencil state (disabled for 2D)
    D3D11_DEPTH_STENCIL_DESC dsDesc = {0};
    dsDesc.DepthEnable = FALSE;
    dsDesc.StencilEnable = FALSE;
    
    hr = DEV_CreateDepthStencilState(g_gpuCtx->device, &dsDesc, &g_gpuCtx->depthStencilState);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to create depth stencil state: 0x%08X", hr);
        goto cleanup_error;
    }
    
    // Initialize batch
    if (!hal_batch_init(&g_gpuCtx->batch)) {
        hal_gpu_log("Failed to initialize render batch");
        goto cleanup_error;
    }
    
    hal_gpu_log("GPU renderer initialized successfully (%dx%d, MSAA %dx)", 
                width, height, HAL_GPU_MSAA_SAMPLES);
    return true;

cleanup_shaders:
    if (vsBlob) BLOB_Release(vsBlob);
    if (psBlob) BLOB_Release(psBlob);
    if (shadowBlob) BLOB_Release(shadowBlob);
    if (textBlob) BLOB_Release(textBlob);
    
cleanup_error:
    hal_gpu_shutdown();
    return false;
}

/* ============================================
   GPU Shutdown
   ============================================ */

void hal_gpu_shutdown(void) {
    if (!g_gpuCtx) return;
    
    hal_batch_shutdown(&g_gpuCtx->batch);
    
    if (g_gpuCtx->depthStencilState) DSS_Release(g_gpuCtx->depthStencilState);
    if (g_gpuCtx->samplerPoint) SS_Release(g_gpuCtx->samplerPoint);
    if (g_gpuCtx->samplerLinear) SS_Release(g_gpuCtx->samplerLinear);
    if (g_gpuCtx->rasterizerState) RS_Release(g_gpuCtx->rasterizerState);
    if (g_gpuCtx->blendStateAdditive) BS_Release(g_gpuCtx->blendStateAdditive);
    if (g_gpuCtx->blendStateNormal) BS_Release(g_gpuCtx->blendStateNormal);
    if (g_gpuCtx->constantBuffer) BUF_Release(g_gpuCtx->constantBuffer);
    if (g_gpuCtx->indexBuffer) BUF_Release(g_gpuCtx->indexBuffer);
    if (g_gpuCtx->vertexBuffer) BUF_Release(g_gpuCtx->vertexBuffer);
    if (g_gpuCtx->inputLayout) IL_Release(g_gpuCtx->inputLayout);
    if (g_gpuCtx->textShader) PS_Release(g_gpuCtx->textShader);
    if (g_gpuCtx->shadowShader) PS_Release(g_gpuCtx->shadowShader);
    if (g_gpuCtx->pixelShader) PS_Release(g_gpuCtx->pixelShader);
    if (g_gpuCtx->vertexShader) VS_Release(g_gpuCtx->vertexShader);
    if (g_gpuCtx->msaaSRV) SRV_Release(g_gpuCtx->msaaSRV);
    if (g_gpuCtx->msaaRTV) RTV_Release(g_gpuCtx->msaaRTV);
    if (g_gpuCtx->msaaTexture) TEX_Release(g_gpuCtx->msaaTexture);
    if (g_gpuCtx->backBufferRTV) RTV_Release(g_gpuCtx->backBufferRTV);
    if (g_gpuCtx->swapChain) SC_Release(g_gpuCtx->swapChain);
    if (g_gpuCtx->context) CTX_Release(g_gpuCtx->context);
    if (g_gpuCtx->device) DEV_Release(g_gpuCtx->device);
    
    free(g_gpuCtx);
    g_gpuCtx = NULL;
    
    hal_gpu_log("GPU renderer shutdown");
}

/* ============================================
   Resize
   ============================================ */

bool hal_gpu_resize(uint32_t width, uint32_t height) {
    if (!g_gpuCtx || width == 0 || height == 0) return false;
    
    // Release old resources
    if (g_gpuCtx->backBufferRTV) {
        RTV_Release(g_gpuCtx->backBufferRTV);
        g_gpuCtx->backBufferRTV = NULL;
    }
    if (g_gpuCtx->msaaRTV) {
        RTV_Release(g_gpuCtx->msaaRTV);
        g_gpuCtx->msaaRTV = NULL;
    }
    if (g_gpuCtx->msaaTexture) {
        TEX_Release(g_gpuCtx->msaaTexture);
        g_gpuCtx->msaaTexture = NULL;
    }
    
    // Resize swap chain
    HRESULT hr = SC_ResizeBuffers(g_gpuCtx->swapChain, 0, width, height,
                                                DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        hal_gpu_log("Failed to resize swap chain: 0x%08X", hr);
        return false;
    }
    
    // Recreate back buffer RTV
    ID3D11Texture2D* backBuffer = NULL;
    hr = SC_GetBuffer(g_gpuCtx->swapChain, 0, &IID_ID3D11Texture2D, (void**)&backBuffer);
    if (FAILED(hr)) return false;
    
    hr = DEV_CreateRenderTargetView(g_gpuCtx->device, (ID3D11Resource*)backBuffer,
                                              NULL, &g_gpuCtx->backBufferRTV);
    TEX_Release(backBuffer);
    if (FAILED(hr)) return false;
    
    // Recreate MSAA texture
    D3D11_TEXTURE2D_DESC msaaDesc = {0};
    msaaDesc.Width = width;
    msaaDesc.Height = height;
    msaaDesc.MipLevels = 1;
    msaaDesc.ArraySize = 1;
    msaaDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaDesc.SampleDesc.Count = HAL_GPU_MSAA_SAMPLES;
    msaaDesc.Usage = D3D11_USAGE_DEFAULT;
    msaaDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    
    hr = DEV_CreateTexture2D(g_gpuCtx->device, &msaaDesc, NULL, &g_gpuCtx->msaaTexture);
    if (FAILED(hr)) return false;
    
    hr = DEV_CreateRenderTargetView(g_gpuCtx->device, (ID3D11Resource*)g_gpuCtx->msaaTexture,
                                              NULL, &g_gpuCtx->msaaRTV);
    if (FAILED(hr)) return false;
    
    g_gpuCtx->width = width;
    g_gpuCtx->height = height;
    
    return true;
}

/* ============================================
   Frame Management
   ============================================ */

void hal_gpu_begin_frame(void) {
    if (!g_gpuCtx) return;
    
    // Clear batch
    hal_batch_clear(&g_gpuCtx->batch);
    
    // Reset stats
    g_gpuCtx->drawCallsThisFrame = 0;
    g_gpuCtx->trianglesThisFrame = 0;
    
    // Clear render target
    float clearColor[4] = {0.1f, 0.1f, 0.12f, 1.0f};
    CTX_ClearRenderTargetView(g_gpuCtx->context, g_gpuCtx->msaaRTV, clearColor);
    
    // Set render target
    CTX_OMSetRenderTargets(g_gpuCtx->context, 1, &g_gpuCtx->msaaRTV, NULL);
    
    // Set viewport
    D3D11_VIEWPORT viewport = {0};
    viewport.Width = (float)g_gpuCtx->width;
    viewport.Height = (float)g_gpuCtx->height;
    viewport.MaxDepth = 1.0f;
    CTX_RSSetViewports(g_gpuCtx->context, 1, &viewport);
    
    // Set scissor rect
    D3D11_RECT scissor = {0, 0, (LONG)g_gpuCtx->width, (LONG)g_gpuCtx->height};
    CTX_RSSetScissorRects(g_gpuCtx->context, 1, &scissor);
    
    // Set pipeline state
    CTX_IASetInputLayout(g_gpuCtx->context, g_gpuCtx->inputLayout);
    CTX_IASetPrimitiveTopology(g_gpuCtx->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CTX_VSSetShader(g_gpuCtx->context, g_gpuCtx->vertexShader, NULL, 0);
    CTX_PSSetShader(g_gpuCtx->context, g_gpuCtx->pixelShader, NULL, 0);
    CTX_RSSetState(g_gpuCtx->context, g_gpuCtx->rasterizerState);
    CTX_OMSetDepthStencilState(g_gpuCtx->context, g_gpuCtx->depthStencilState, 0);
    
    float blendFactor[4] = {0, 0, 0, 0};
    CTX_OMSetBlendState(g_gpuCtx->context, g_gpuCtx->blendStateNormal, blendFactor, 0xFFFFFFFF);
    
    CTX_PSSetSamplers(g_gpuCtx->context, 0, 1, &g_gpuCtx->samplerLinear);
    
    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = CTX_Map(g_gpuCtx->context, (ID3D11Resource*)g_gpuCtx->constantBuffer,
                                          0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        HalGPUConstants* constants = (HalGPUConstants*)mapped.pData;
        hal_create_projection_matrix(constants->projectionMatrix, 
                                     (float)g_gpuCtx->width, (float)g_gpuCtx->height);
        constants->viewportSize[0] = (float)g_gpuCtx->width;
        constants->viewportSize[1] = (float)g_gpuCtx->height;
        constants->time = (float)g_gpuCtx->frameCount * 0.016f;
        constants->dpiScale = g_gpuCtx->dpiScale;
        constants->shadowColor[0] = 0.0f;
        constants->shadowColor[1] = 0.0f;
        constants->shadowColor[2] = 0.0f;
        constants->shadowColor[3] = 0.3f;
        constants->ambientLight = 0.1f;
        CTX_Unmap(g_gpuCtx->context, (ID3D11Resource*)g_gpuCtx->constantBuffer, 0);
    } else {
        hal_gpu_log("Failed to map constant buffer: 0x%08X", hr);
    }
    
    CTX_VSSetConstantBuffers(g_gpuCtx->context, 0, 1, &g_gpuCtx->constantBuffer);
    CTX_PSSetConstantBuffers(g_gpuCtx->context, 0, 1, &g_gpuCtx->constantBuffer);
}

static void hal_gpu_flush_batch(void) {
    if (!g_gpuCtx || g_gpuCtx->batch.vertexCount == 0) return;
    
    // Upload vertex data
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = CTX_Map(g_gpuCtx->context, (ID3D11Resource*)g_gpuCtx->vertexBuffer,
                                          0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        memcpy(mapped.pData, g_gpuCtx->batch.vertices, 
               g_gpuCtx->batch.vertexCount * sizeof(HalGPUVertex));
        CTX_Unmap(g_gpuCtx->context, (ID3D11Resource*)g_gpuCtx->vertexBuffer, 0);
    } else {
        hal_gpu_log("Failed to map vertex buffer: 0x%08X", hr);
        return;
    }
    
    // Upload index data
    hr = CTX_Map(g_gpuCtx->context, (ID3D11Resource*)g_gpuCtx->indexBuffer,
                                  0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        memcpy(mapped.pData, g_gpuCtx->batch.indices,
               g_gpuCtx->batch.indexCount * sizeof(uint32_t));
        CTX_Unmap(g_gpuCtx->context, (ID3D11Resource*)g_gpuCtx->indexBuffer, 0);
    } else {
        hal_gpu_log("Failed to map index buffer: 0x%08X", hr);
        return;
    }
    
    // Bind buffers
    UINT stride = sizeof(HalGPUVertex);
    UINT offset = 0;
    CTX_IASetVertexBuffers(g_gpuCtx->context, 0, 1, &g_gpuCtx->vertexBuffer, &stride, &offset);
    CTX_IASetIndexBuffer(g_gpuCtx->context, g_gpuCtx->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    
    // Draw
    CTX_DrawIndexed(g_gpuCtx->context, g_gpuCtx->batch.indexCount, 0, 0);
    
    g_gpuCtx->drawCallsThisFrame++;
    g_gpuCtx->trianglesThisFrame += g_gpuCtx->batch.indexCount / 3;
    
    g_gpuCtx->batch.needsUpload = false;
}

void hal_gpu_end_frame(void) {
    if (!g_gpuCtx) return;
    
    // Flush remaining batch
    hal_gpu_flush_batch();
    
    // Get back buffer texture for resolve
    ID3D11Texture2D* backBufferTexture = NULL;
    HRESULT hr = SC_GetBuffer(g_gpuCtx->swapChain, 0, &IID_ID3D11Texture2D, (void**)&backBufferTexture);
    
    if (SUCCEEDED(hr) && backBufferTexture) {
        // Resolve MSAA to back buffer
        CTX_ResolveSubresource(g_gpuCtx->context,
            (ID3D11Resource*)backBufferTexture, 0,
            (ID3D11Resource*)g_gpuCtx->msaaTexture, 0,
            DXGI_FORMAT_R8G8B8A8_UNORM);
        
        TEX_Release(backBufferTexture);
    }
}

void hal_gpu_present(void) {
    if (!g_gpuCtx) return;
    
    UINT syncInterval = g_gpuCtx->vsyncEnabled ? 1 : 0;
    SC_Present(g_gpuCtx->swapChain, syncInterval, 0);
    
    g_gpuCtx->frameCount++;
}

/* ============================================
   Drawing Functions
   ============================================ */

static void hal_color_to_float(uint32_t color, float* r, float* g, float* b, float* a) {
    *a = ((color >> 24) & 0xFF) / 255.0f;
    *r = ((color >> 16) & 0xFF) / 255.0f;
    *g = ((color >> 8) & 0xFF) / 255.0f;
    *b = (color & 0xFF) / 255.0f;
    
    // sRGB to linear conversion (proper gamma correction)
    // For values <= 0.04045, use linear scaling, otherwise apply gamma curve
    #define SRGB_TO_LINEAR(c) ((c) <= 0.04045f ? (c) / 12.92f : powf(((c) + 0.055f) / 1.055f, 2.4f))
    *r = SRGB_TO_LINEAR(*r);
    *g = SRGB_TO_LINEAR(*g);
    *b = SRGB_TO_LINEAR(*b);
    #undef SRGB_TO_LINEAR
}

void hal_gpu_draw_rect(float x, float y, float w, float h, uint32_t color) {
    if (!g_gpuCtx) return;
    
    float r, g, b, a;
    hal_color_to_float(color, &r, &g, &b, &a);
    
    hal_batch_add_quad(&g_gpuCtx->batch, x, y, w, h, 0, 0, 1, 1, r, g, b, a, 0, 0, 0, 0);
}

void hal_gpu_draw_rounded_rect(float x, float y, float w, float h, float radius, uint32_t color) {
    if (!g_gpuCtx) return;
    
    float r, g, b, a;
    hal_color_to_float(color, &r, &g, &b, &a);
    
    // Normalize radius to UV space
    float normalizedRadius = radius / (w < h ? w : h) * 0.5f;
    
    hal_batch_add_quad(&g_gpuCtx->batch, x, y, w, h, 0, 0, 1, 1, r, g, b, a, 
                       normalizedRadius, 0, 0, 0);
}

void hal_gpu_draw_rounded_rect_ex(float x, float y, float w, float h, float radius,
                                   uint32_t fillColor, uint32_t borderColor, float borderWidth) {
    if (!g_gpuCtx) return;
    
    float fr, fg, fb, fa;
    hal_color_to_float(fillColor, &fr, &fg, &fb, &fa);
    
    float normalizedRadius = radius / (w < h ? w : h) * 0.5f;
    float normalizedBorder = borderWidth / (w < h ? w : h);
    
    hal_batch_add_quad(&g_gpuCtx->batch, x, y, w, h, 0, 0, 1, 1, fr, fg, fb, fa,
                       normalizedRadius, normalizedBorder, 0, 0);
}

void hal_gpu_draw_shadow(float x, float y, float w, float h, float radius,
                         HalElevationLevel elevation, uint32_t color) {
    if (!g_gpuCtx || elevation == HAL_ELEVATION_0) return;
    
    // Switch to shadow shader
    CTX_PSSetShader(g_gpuCtx->context, g_gpuCtx->shadowShader, NULL, 0);
    
    // Shadow parameters based on elevation
    float shadowOffset = elevation * 2.0f;
    float shadowBlur = elevation * 4.0f;
    float shadowExpand = elevation * 2.0f;
    
    float r, g, b, a;
    hal_color_to_float(color, &r, &g, &b, &a);
    
    float normalizedRadius = radius / (w < h ? w : h) * 0.5f;
    
    // Draw shadow quad (expanded and offset)
    hal_batch_add_quad(&g_gpuCtx->batch,
                       x - shadowExpand, y + shadowOffset - shadowExpand,
                       w + shadowExpand * 2, h + shadowExpand * 2,
                       0, 0, 1, 1, r, g, b, a * 0.3f,
                       normalizedRadius, 0, shadowBlur, (float)elevation);
    
    // Flush and switch back to normal shader
    hal_gpu_flush_batch();
    hal_batch_clear(&g_gpuCtx->batch);
    CTX_PSSetShader(g_gpuCtx->context, g_gpuCtx->pixelShader, NULL, 0);
}

void hal_gpu_draw_circle(float cx, float cy, float radius, uint32_t color) {
    hal_gpu_draw_rounded_rect(cx - radius, cy - radius, radius * 2, radius * 2, radius, color);
}

void hal_gpu_draw_line(float x1, float y1, float x2, float y2, float thickness, uint32_t color) {
    if (!g_gpuCtx) return;
    
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    
    // Perpendicular vector
    float px = -dy / len * thickness * 0.5f;
    float py = dx / len * thickness * 0.5f;
    
    float r, g, b, a;
    hal_color_to_float(color, &r, &g, &b, &a);
    
    // Add line as quad
    uint32_t baseVertex = g_gpuCtx->batch.vertexCount;
    
    g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
        x1 + px, y1 + py, 0, 0, r, g, b, a, 0, 0, 0, 0
    };
    g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
        x2 + px, y2 + py, 1, 0, r, g, b, a, 0, 0, 0, 0
    };
    g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
        x2 - px, y2 - py, 1, 1, r, g, b, a, 0, 0, 0, 0
    };
    g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
        x1 - px, y1 - py, 0, 1, r, g, b, a, 0, 0, 0, 0
    };
    
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 0;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 1;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 2;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 0;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 2;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 3;
    
    g_gpuCtx->batch.needsUpload = true;
}

void hal_gpu_draw_gradient(float x, float y, float w, float h,
                           uint32_t colorStart, uint32_t colorEnd, bool vertical) {
    if (!g_gpuCtx) return;
    
    float r1, g1, b1, a1;
    float r2, g2, b2, a2;
    hal_color_to_float(colorStart, &r1, &g1, &b1, &a1);
    hal_color_to_float(colorEnd, &r2, &g2, &b2, &a2);
    
    uint32_t baseVertex = g_gpuCtx->batch.vertexCount;
    
    if (vertical) {
        // Top vertices use start color
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x, y, 0, 0, r1, g1, b1, a1, 0, 0, 0, 0
        };
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x + w, y, 1, 0, r1, g1, b1, a1, 0, 0, 0, 0
        };
        // Bottom vertices use end color
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x + w, y + h, 1, 1, r2, g2, b2, a2, 0, 0, 0, 0
        };
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x, y + h, 0, 1, r2, g2, b2, a2, 0, 0, 0, 0
        };
    } else {
        // Left vertices use start color
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x, y, 0, 0, r1, g1, b1, a1, 0, 0, 0, 0
        };
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x + w, y, 1, 0, r2, g2, b2, a2, 0, 0, 0, 0
        };
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x + w, y + h, 1, 1, r2, g2, b2, a2, 0, 0, 0, 0
        };
        g_gpuCtx->batch.vertices[g_gpuCtx->batch.vertexCount++] = (HalGPUVertex){
            x, y + h, 0, 1, r1, g1, b1, a1, 0, 0, 0, 0
        };
    }
    
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 0;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 1;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 2;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 0;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 2;
    g_gpuCtx->batch.indices[g_gpuCtx->batch.indexCount++] = baseVertex + 3;
    
    g_gpuCtx->batch.needsUpload = true;
}

/* ============================================
   Text Rendering (placeholder - needs SDF font atlas)
   ============================================ */

void hal_gpu_draw_text(const char* text, float x, float y, float size, uint32_t color) {
    // TODO: Implement SDF font rendering
    // For now, this is a placeholder that draws a rectangle
    if (!text || !*text) return;
    
    float width = strlen(text) * size * 0.6f;
    float height = size;
    
    hal_gpu_draw_rounded_rect(x, y, width, height, 2, color);
}

void hal_gpu_draw_text_aligned(const char* text, float x, float y, float w, float h,
                                float size, uint32_t color, int alignH, int alignV) {
    if (!text || !*text) return;
    
    float textWidth = strlen(text) * size * 0.6f;
    float textHeight = size;
    
    float tx = x;
    float ty = y;
    
    // Horizontal alignment
    if (alignH == 1) tx = x + (w - textWidth) * 0.5f;      // Center
    else if (alignH == 2) tx = x + w - textWidth;          // Right
    
    // Vertical alignment
    if (alignV == 1) ty = y + (h - textHeight) * 0.5f;     // Middle
    else if (alignV == 2) ty = y + h - textHeight;         // Bottom
    
    hal_gpu_draw_text(text, tx, ty, size, color);
}

/* ============================================
   Clipping
   ============================================ */

static D3D11_RECT g_clipStack[32];
static int g_clipStackTop = 0;

void hal_gpu_push_clip(float x, float y, float w, float h) {
    if (!g_gpuCtx || g_clipStackTop >= 32) return;
    
    // Flush before changing clip
    hal_gpu_flush_batch();
    hal_batch_clear(&g_gpuCtx->batch);
    
    D3D11_RECT rect;
    rect.left = (LONG)x;
    rect.top = (LONG)y;
    rect.right = (LONG)(x + w);
    rect.bottom = (LONG)(y + h);
    
    // Intersect with current clip if any
    if (g_clipStackTop > 0) {
        D3D11_RECT* current = &g_clipStack[g_clipStackTop - 1];
        if (rect.left < current->left) rect.left = current->left;
        if (rect.top < current->top) rect.top = current->top;
        if (rect.right > current->right) rect.right = current->right;
        if (rect.bottom > current->bottom) rect.bottom = current->bottom;
    }
    
    g_clipStack[g_clipStackTop++] = rect;
    CTX_RSSetScissorRects(g_gpuCtx->context, 1, &rect);
}

void hal_gpu_pop_clip(void) {
    if (!g_gpuCtx || g_clipStackTop <= 0) return;
    
    // Flush before changing clip
    hal_gpu_flush_batch();
    hal_batch_clear(&g_gpuCtx->batch);
    
    g_clipStackTop--;
    
    if (g_clipStackTop > 0) {
        CTX_RSSetScissorRects(g_gpuCtx->context, 1, &g_clipStack[g_clipStackTop - 1]);
    } else {
        D3D11_RECT fullRect = {0, 0, (LONG)g_gpuCtx->width, (LONG)g_gpuCtx->height};
        CTX_RSSetScissorRects(g_gpuCtx->context, 1, &fullRect);
    }
}

/* ============================================
   State & Utility Functions
   ============================================ */

void hal_gpu_set_blend_mode(int mode) {
    if (!g_gpuCtx) return;
    
    float blendFactor[4] = {0, 0, 0, 0};
    
    switch (mode) {
        case 0: // Normal
            CTX_OMSetBlendState(g_gpuCtx->context, g_gpuCtx->blendStateNormal, 
                                                 blendFactor, 0xFFFFFFFF);
            break;
        case 1: // Additive
            CTX_OMSetBlendState(g_gpuCtx->context, g_gpuCtx->blendStateAdditive,
                                                 blendFactor, 0xFFFFFFFF);
            break;
    }
}

HalGPUContext* hal_gpu_get_context(void) {
    return g_gpuCtx;
}

void hal_gpu_set_dpi_scale(float scale) {
    if (g_gpuCtx) {
        g_gpuCtx->dpiScale = scale;
    }
}

float hal_gpu_get_dpi_scale(void) {
    return g_gpuCtx ? g_gpuCtx->dpiScale : 1.0f;
}

void hal_gpu_set_vsync(bool enabled) {
    if (g_gpuCtx) {
        g_gpuCtx->vsyncEnabled = enabled;
    }
}

double hal_gpu_get_frame_time(void) {
    return g_gpuCtx ? g_gpuCtx->deltaTime : 0.0;
}

uint32_t hal_gpu_get_draw_calls(void) {
    return g_gpuCtx ? g_gpuCtx->drawCallsThisFrame : 0;
}

/************************
   Program : Kotak 3D OpenGL
   Anggota Kelompok :
           1. Muzaki Fadil   : 10108414
           2. Wanda          : 10108419
           3. Giri Supangkat : 10108423
           4. Ringga Anggiat : 10107569
  ************************/

// include file header dasar windows dan file header Direct3D
#define WIN32_LEAN_AND_MEAN       
#include <windows.h>

//Include header Direct3D 
#include <d3dx9.h>

// resolusi layar
#define APP_WIDTH 800
#define APP_HEIGHT 600

//membersihkan objek COM
#define RELEASE_COM_OBJECT(i) if(i!=NULL&& i) i->Release();

//deklarasi global

LPDIRECT3D9 d3d=NULL;                       //Pointer antarmuka Direct3D 
LPDIRECT3DDEVICE9 device=NULL;              //Pointer ke kelas device
LPDIRECT3DVERTEXBUFFER9 vertex_buffer=NULL; //pointer to the vertex buffer
LPDIRECT3DTEXTURE9 texture_1=NULL;          //tekstur

// nilai konstan
#define CUBE_ACCEL 1.0025
#define CUBE_START_SPEED 0.02
#define CUBE_MAX_SPEED 0.10
#define CAMERA_DISTANCE 19.5
#define AMBIENT_BRIGHTNESS 140
#define TEXTURE_SIZE 1024
#define VERTICAL_VIEWFIELD_DEGREES 45.0
#define FRUSTUM_NEAR_Z 1.0f
#define FRUSTUM_FAR_Z 100.0f
#define LIGHT_DISTANCE 40.0f
#define LIGHT_RANGE 60.0f
#define LIGHT_CONE_RADIANS_INNER 0.405f; 
#define LIGHT_CONE_RADIANS_OUTER 0.425f; 
#define TEXTURE_FILE_NAME "plaintex.png"
#define CUBE_VERTICES 24

//
// Fungsi prototipe
//
void init_device(HWND hWnd);    
void render();  
void cleanup(); 
void init_graphics(); 
void init_lights();   
void restore_surfaces(HWND hWnd);

//Direct3D makes us declare out own vertex type... we include the obvious i.e.
// coordinates in 3 dimensions, normal vector, and U/V for texture mapping.
struct MYVERTEXTYPE {FLOAT X, Y, Z; D3DVECTOR NORMAL; FLOAT U, V;};
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// the WindowProc function prototype
LRESULT CALLBACK 
  WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{

  WNDCLASSEX wc;

  ZeroMemory(&wc, sizeof(WNDCLASSEX));

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)WindowProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  //MinGW uses 8-bit characters (ASCII) by default
  wc.lpszClassName = "WindowClass";

  RegisterClassEx(&wc);

  HWND hWnd = CreateWindowEx(0, "WindowClass", "MinGW3D",
                 WS_EX_TOPMOST | WS_SYSMENU, 0, 0, APP_WIDTH,
                 APP_HEIGHT, NULL, NULL, hInstance, NULL);

  ShowWindow(hWnd, nCmdShow);

  // set up and initialize Direct3D
  init_device(hWnd);

  MSG msg;

  //Game loop...
  // This is basically a "Do Events" action followed by a call to "render()"
  while(TRUE)
  {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        render();

  }

  // clean up DirectX and COM
  cleanup();

  return msg.wParam;

}

//
// This is the main message handler for the program
//
LRESULT CALLBACK WindowProc
	(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
        case WM_SYSCOMMAND: //screen saver... exit to allow
         if(wParam==SC_SCREENSAVE)
         {
          PostQuitMessage(0);
          return 0;
         }
  	 break;
        case WM_DESTROY:           
         PostQuitMessage(0);
         return 0;
        break;

        case WM_SETFOCUS:
         restore_surfaces(hWnd);				
        break;

  }
  return DefWindowProc (hWnd, message, wParam, lParam);
}

// This function initializes and prepares Direct3D for use
void init_device(HWND hWnd)
{

  d3d = Direct3DCreate9(D3D_SDK_VERSION);

  D3DPRESENT_PARAMETERS d3dpp;

  ZeroMemory(&d3dpp, sizeof(d3dpp));
  d3dpp.Windowed = TRUE;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.hDeviceWindow = hWnd;
  d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
  d3dpp.BackBufferWidth = APP_WIDTH;
  d3dpp.BackBufferHeight = APP_HEIGHT;

  // Device Creation
  d3d->CreateDevice(
      D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL,
      hWnd,
      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
      &d3dpp,
      &device);

  init_graphics();
  init_lights();   

  device->SetRenderState(D3DRS_LIGHTING, TRUE);  // Turn on the 3D lighting
  device->SetRenderState(D3DRS_ZENABLE, TRUE);   // Turn on the z-buffer

  device->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);

  // ambient light
  device->SetRenderState(D3DRS_AMBIENT, 
   D3DCOLOR_XRGB(AMBIENT_BRIGHTNESS,AMBIENT_BRIGHTNESS,AMBIENT_BRIGHTNESS));


  // set the view transform
  D3DXMATRIX matView;    // the view transform matrix

  D3DXVECTOR3 camera(0.0f, 0.0f, -CAMERA_DISTANCE);
  D3DXVECTOR3 lookat(0.0f, 0.0f, 0.0f);
  D3DXVECTOR3 upvector(0.0f, 1.0f, 0.0f);

  D3DXMatrixLookAtLH(&matView,
    &camera,         // the camera position
    &lookat,         // the look-at position
    &upvector);      // the up direction

  device->SetTransform(D3DTS_VIEW, &matView);

  // This is the projection transform.. the last parameter to the call 
  // is the point in positive Z-dimension that things become invisible
  D3DXMATRIX matProjection;  

  D3DXMatrixPerspectiveFovLH
    (&matProjection,
     D3DXToRadian(VERTICAL_VIEWFIELD_DEGREES),  
     (FLOAT)APP_WIDTH / (FLOAT)APP_HEIGHT, 
     FRUSTUM_NEAR_Z,     //Defined near top of file    
     FRUSTUM_FAR_Z);   
 
  device->SetTransform(D3DTS_PROJECTION, &matProjection);

  device->SetFVF(CUSTOMFVF);


}

// this is the function used to render a single frame
void render()
{

  static float index = 0.0 ;

  //Clear output buffer and vertex buffer 
  device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
  device->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

  device->BeginScene();

  //
  //Index is used to spin the main cube... a rotation of index is applied in
  // each dimension to make things interesting
  //
  static float speed = CUBE_START_SPEED;
  speed *= CUBE_ACCEL;
   
  if(speed>CUBE_MAX_SPEED) speed=CUBE_MAX_SPEED;
   
  index+=speed;  

  D3DXMATRIX matRotateX;

  D3DXMatrixRotationYawPitchRoll(&matRotateX,index,index,index);

  // set the world transform
  device->SetTransform(D3DTS_WORLDMATRIX(0), &(matRotateX));    
  device->SetStreamSource(0, vertex_buffer, 0, sizeof(MYVERTEXTYPE));
    
  device->SetTexture(0, texture_1);
    
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);	
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 4, 2);
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 8, 2);
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 12, 2);
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 16, 2);
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 20, 2); 

  device->EndScene(); 
  device->Present(NULL, NULL, NULL, NULL) ;

}

// this is the function that cleans up Direct3D and COM
void cleanup()
{
  RELEASE_COM_OBJECT(vertex_buffer)
  RELEASE_COM_OBJECT(texture_1)
  RELEASE_COM_OBJECT(device)
  RELEASE_COM_OBJECT(d3d)    
}


// This is the function that defines the 3D cube
void init_graphics()
{

  D3DXCreateTextureFromFileEx(
   device,
   TEXTURE_FILE_NAME, 		
   TEXTURE_SIZE,TEXTURE_SIZE,
   D3DX_DEFAULT,0,D3DFMT_X8R8G8B8,D3DPOOL_MANAGED,
   D3DX_DEFAULT,D3DX_DEFAULT,0, NULL, NULL, &texture_1
  );


  //
  // This defines the shape of our single large polyhedron
  //
  MYVERTEXTYPE demo_vertices[] =
  {
       //
       //Each of these groups-of-four is a face of our solid
       //

        { -5.0f, 5.0f, -5.0f, 0, 0, -1, 0, 0 },  // Tampak Depan  
        { 5.0f, 5.0f, -5.0f, 0, 0, -1, 1, 0 },
        { -5.0f, -5.0f, -5.0f, 0, 0, -1, 0, 1 },
        { 5.0f, -5.0f, -5.0f, 0, 0, -1, 1, 1 },

        { 5.0f, 5.0f, 5.0f, 0, 0, 1, 0, 0 },     // Tampak Belakang
        { -5.0f, 5.0f, 5.0f, 0, 0, 1, 1, 0 },	  
        { 5.0f, -5.0f, 5.0f, 0, 0, 1, 0, 1 },    
        { -5.0f, -5.0f, 5.0f, 0, 0, 1, 1, 1 },   

        { -5.0f, 5.0f, 5.0f, 0, 1, 0, 0, 0 },    // Tampak Atas
        { 5.0f, 5.0f, 5.0f, 0, 1, 0, 1, 0 },
        { -5.0f, 5.0f, -5.0f, 0, 1, 0, 0, 1 },
        { 5.0f, 5.0f, -5.0f, 0, 1, 0, 1, 1 },

        { 5.0f, 5.0f, -5.0f, 1, 0, 0, 0, 0 },    // Tampak Kanan
        { 5.0f, 5.0f, 5.0f, 1, 0, 0, 1, 0 },
        { 5.0f, -5.0f, -5.0f, 1, 0, 0, 0, 1 },
        { 5.0f, -5.0f, 5.0f, 1, 0, 0, 1, 1 },

        { -5.0f, -5.0f, -5.0f, 0, -1, 0, 0, 0 },  //Tampak Bawah
        { 5.0f, -5.0f, -5.0f, 0, -1, 0, 1, 0 },
        { -5.0f, -5.0f, 5.0f, 0, -1, 0, 0, 1 },
        { 5.0f, -5.0f, 5.0f, 0, -1, 0, 1, 1 },

        { -5.0f, 5.0f, 5.0f, -1, 0, 0, 0, 0 },    //Tampak Kiri
        { -5.0f, 5.0f, -5.0f, -1, 0, 0, 1, 0 },
        { -5.0f, -5.0f, 5.0f, -1, 0, 0, 0, 1 },
        { -5.0f, -5.0f, -5.0f, -1, 0, 0, 1, 1 },

  };    
 
  // COM creation of vertex buffer
  device->CreateVertexBuffer(
	   CUBE_VERTICES*sizeof(MYVERTEXTYPE),		//Cube has 24 vertices
           0,
           CUSTOMFVF,
           D3DPOOL_MANAGED,
           &vertex_buffer,
           NULL);

  VOID* pVoid;    // a void pointer

  // Lock vertex_buffer and load the vertices into it
  vertex_buffer->Lock(0, 0, (void**)&pVoid, 0);
  memcpy(pVoid, demo_vertices, sizeof(demo_vertices));
  vertex_buffer->Unlock();

}

// Gets called after we leave then return to this program; handled by
//  throwing away our old COM objects and re-doing initialization
void restore_surfaces(HWND hWnd)
{
   cleanup();
   init_device(hWnd);
}

// Sets up the lights and "material" parameter
void init_lights()
{

  D3DMATERIAL9 material;    
	
  D3DVECTOR look = {0.00f, 0.0f, 1.0f};    // the direction of the light

  // the location of the light
  D3DVECTOR at = {0.0f, 0.0f, -LIGHT_DISTANCE};     

  D3DLIGHT9 flashlight;
  ZeroMemory(&flashlight, sizeof(D3DLIGHT9));   

  flashlight.Type = D3DLIGHT_SPOT;
  flashlight.Diffuse.r = flashlight.Ambient.r = 1.0f;	//1.0 = max value
  flashlight.Diffuse.g = flashlight.Ambient.g = 1.0f;
  flashlight.Diffuse.b = flashlight.Ambient.b = 1.0f;
  flashlight.Diffuse.a = flashlight.Ambient.a = 1.0f;
  flashlight.Range = LIGHT_RANGE;
  flashlight.Position	= at;
  flashlight.Direction = look;	
  flashlight.Phi = LIGHT_CONE_RADIANS_OUTER;		
  flashlight.Theta = LIGHT_CONE_RADIANS_INNER;		
  flashlight.Attenuation0 = 0.0f;        //Constant attenuation
  flashlight.Attenuation1 = 0.01f;       //Linear attenuation
  flashlight.Attenuation2 = 0.0f;        //Quadratic attenuation
  flashlight.Falloff = 1.0f;        	 //Microsoft recommends linear falloff
  device->SetLight(0, &flashlight);    
  device->LightEnable(0, TRUE);    
  ZeroMemory(&material, sizeof(D3DMATERIAL9));   
  material.Diffuse.r = material.Ambient.r = 1.0f;    
  material.Diffuse.g = material.Ambient.g = 1.0f;  
  material.Diffuse.b = material.Ambient.b = 1.0f;  
  material.Diffuse.a = material.Ambient.a = 1.0f;  
  device->SetMaterial(&material);    

}

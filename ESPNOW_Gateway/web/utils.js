var isDarkMode = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', e => {
    applyDarkMode(e.matches)
})

const applyDarkMode = (dark = false) => {
    document.getElementsByTagName('html')[0].setAttribute('data-bs-theme', dark ? 'dark' : 'light')
}
applyDarkMode(isDarkMode)


const enurl = encodeURIComponent

const goTo = (url) => location.href = location.href.split('/').slice(0, -1).join('/') + url


const isMobile = window.innerWidth < 768

const { Loading, Confirm } = Notiflix

Confirm.init({ width: '400px', titleColor: 'rgb(var(--bs-primary-rgb))', okButtonBackground: 'rgb(var(--bs-primary-rgb))', titleFontSize: '1.3em', messageColor: 'var(--bs-body-color)', messageFontSize: '1em', buttonFontSize: '1em' })
Loading.init({ svgColor: 'var(--bs-body-color)' })
Notiflix.Notify.init({ useFontAwesome: true, position: isMobile ? 'center-bottom' : 'right-top', timeout: isMobile ? 3000 : 10000, width: '350px', fontSize: '1.1em' })

const $main = document.getElementById('main')

const GET = (url) => fetch(url).then(response => response.json())
const POST = (url, data) => fetch(url, { method: 'POST', body: data }).then(response => response.json())

const emptyNode = () => {
    $main.querySelector('.col-node-empty').classList.remove('d-none')
}

const notify = (type, ...message) => {
    Notiflix.Notify[type](message.join(' '))
}

const alertReload = (title = "Error", message = "") => {
    Confirm.show(title, message, 'Reload', 'Close', () => { location.reload() });
}

const json2query = (json) => {
    return Object.keys(json).map(key => enurl(key) + '=' + enurl(json[key])).join('&')
}

const isPlainObject = (obj) => {
    if (obj === null || typeof obj !== 'object' || Array.isArray(obj)) {
      return false;
    }
    let proto = Object.getPrototypeOf(obj);
    while (proto) {
      if (proto === Object.prototype) {
        return true;
      }
      proto = Object.getPrototypeOf(proto);
    }
    return false;
  }
  

const duplicateObject = (obj) => {
    let nObj = {}
    for (let key in obj) {
        if (typeof obj[key] == 'function')
            continue
        if (typeof obj[key] === 'object' && isPlainObject(obj[key])) {
            nObj[key] = duplicateObject(obj[key])
        } else {
            nObj[key] = obj[key]
        }
    }
    return nObj
}





let baseURI = ""
baseURI = "http://192.168.0.88"



const GetDevices = async () => {
    Loading.circle()
    return POST(baseURI + '/devices')
    .then(data => {
        Loading.remove()
        console.log(data)
        return data
    })
    .catch(error => {
        Loading.remove()
        alertReload("Error fetch devices", error.message)
        emptyNode()
        return null
    })
}